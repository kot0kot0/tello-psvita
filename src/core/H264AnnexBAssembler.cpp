#include "H264AnnexBAssembler.hpp"
#include <cstring>

bool H264AnnexBAssembler::isStartCode(const uint8_t* p) {
    // Start code (0x00 00 00 01) を検出する
    return p[0] == 0x00 && p[1] == 0x00 &&
           p[2] == 0x00 && p[3] == 0x01;
}

void H264AnnexBAssembler::push(const uint8_t* data, size_t size) {
    m_buffer.insert(m_buffer.end(), data, data + size);
}

bool H264AnnexBAssembler::popAccessUnit(VideoPacket& out) {
    // 1. 同期チェック：先頭が Start Code (00 00 00 01) か確認
    if (m_buffer.size() < 4) return false;
    if (!isStartCode(m_buffer.data())) {
        // 先頭が壊れている場合、次の Start Code まで検索して捨てる
        size_t syncPos = 1;
        while (syncPos + 4 <= m_buffer.size() && !isStartCode(&m_buffer[syncPos])) {
            syncPos++;
        }
        m_buffer.erase(m_buffer.begin(), m_buffer.begin() + syncPos);
        m_analyzedSize = 0;
        return false;
    }

    // 2. 次の Start Code を探す (m_analyzedSize を使って効率化)
    // 少なくとも現在の Start Code (idx=0..3) は飛ばすので 4 から開始
    size_t pos = std::max((size_t)4, m_analyzedSize);
    bool foundNext = false;

    while (pos + 4 <= m_buffer.size()) {
        if (isStartCode(&m_buffer[pos])) {
            foundNext = true;
            break;
        }
        pos++;
    }

    // 次が見つからない場合は、どこまで探したか記録して終了
    if (!foundNext) {
        m_analyzedSize = pos; 
        return false;
    }

    // --- ここから下は「1つの NAL ユニットが完全に溜まった」状態 ---

    // 3. NAL ユニットの特定 (安全のためサイズ確認後にアクセス)
    if (m_buffer.size() < 5) return false; // 万が一のためのガード
    const uint8_t nalHeader = m_buffer[4];
    const uint8_t nalType = nalHeader & 0x1F;
    const size_t nalSize = pos;

    bool ret = false;

    // 元のロジックに従い、各NALタイプを処理
    if (nalType == 7) { // SPS
        m_sps.assign(m_buffer.begin(), m_buffer.begin() + nalSize);
    }
    else if (nalType == 8) { // PPS
        m_pps.assign(m_buffer.begin(), m_buffer.begin() + nalSize);
    }
    else if (nalType == 5) { // IDR
        m_idr.assign(m_buffer.begin(), m_buffer.begin() + nalSize);
    }
    else if (nalType == 1) { // P/B
        out.data.assign(m_buffer.begin(), m_buffer.begin() + nalSize);
        out.isKeyFrame = false;
        ret = true;
    }

    // キーフレームが完成したかチェック
    if (!m_sps.empty() && !m_pps.empty() && !m_idr.empty()) {
        out.data.clear();
        out.data.reserve(m_sps.size() + m_pps.size() + m_idr.size());
        out.data.insert(out.data.end(), m_sps.begin(), m_sps.end());
        out.data.insert(out.data.end(), m_pps.begin(), m_pps.end());
        out.data.insert(out.data.end(), m_idr.begin(), m_idr.end());
        out.isKeyFrame = true;

        m_sps.clear();
        m_pps.clear();
        m_idr.clear();
        ret = true;
    }

    // 4. 解析した NAL ユニットを削除し、解析状態をリセット
    m_buffer.erase(m_buffer.begin(), m_buffer.begin() + nalSize);
    m_analyzedSize = 4; // 次回、新しい先頭の Start Code をスキップして開始

    return ret;
}