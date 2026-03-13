#include <vector>
#include <span>
#include <optional>
#include <cstdint>
#include <system_error>
#include "msg.h"
#include "serialization.h"

namespace network{
// Парсер, накапливающий байты и возвращающий готовый фрейм
class FrameParser {
    std::vector<char> buffer_;
public:
    void append_data(std::span<const char> data) {
        buffer_.insert(buffer_.end(), data.begin(), data.end());
    }

    // Пытается извлечь один полный фрейм из начала буфера.
    // Возвращает payload (сырые данные сообщения) и очищает буфер от обработанных байт.
    // В случае ошибки заполняет ec.
    std::optional<std::pair<MessageType,std::vector<char>>> extract_frame(std::error_code& ec) {
        // Здесь должен быть анализ разных форматов.
        // Для примера реализуем только SizeBefore (2 байта длины).
        MessageType msg;
        std::span buf_loc = std::span(buffer_);
        if (auto msg_opt = parse_msg_type(buf_loc);
            msg_opt.has_value()) {
            return std::nullopt;  // недостаточно данных
        }
        else msg = msg_opt.value();
        buf_loc = buf_loc.subspan(msg_type_to_text(msg,ec).size());
        switch (msg)
        {
            case MessageType::SizeBefore:{
                if(buf_loc.size()<sizeof(uint64_t)){
                    ec.clear();
                    return std::nullopt;
                }
                uint16_t len = (static_cast<uint16_t>(buffer_[0]) << 8) |
                                static_cast<uint16_t>(buffer_[1]);
                if (buffer_.size() < 2 + len) {
                    return std::nullopt;  // ещё не все данные получены
                }
                std::vector<char> payload(
                        std::make_move_iterator(buffer_.begin() + 2),
                        std::make_move_iterator(buffer_.begin() + 2 + len));
                return std::make_pair(msg,std::move(payload));
            }
            default:
                break;
        }
    }  
};
}