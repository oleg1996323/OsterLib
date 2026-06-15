#pragma once
#include <netdb.h>
#include "commonsocket.h"
#include <sys/stat.h>
#include <string>
#include "serialization.h"

class FileSender{
    std::shared_ptr<int> fd_;
    uint64_t offset_ = 0;
    uint32_t chunk_sz_ = 1024;
    public:
    FileSender(
        const std::string& file_path,
        std::error_code& err)
    {
        if(!fs::exists(file_path))
            err = std::make_error_code(std::errc::no_such_file_or_directory);
    }
    FileSender(const FileSender& other):
        fd_(other.fd_),
        offset_(other.offset_),
        chunk_sz_(other.chunk_sz_){}
    virtual ~FileSender(){
        if(fd_)
            close(*fd_);
    }
    bool set_offset(uint64_t offset){

    }
    bool set_chunk_size(uint32_t chunk){

    }
    std::pair<double,double> send(std::error_code& err){

    }
    virtual void callback(){}
};

bool send_file_with_progress(int socket_fd, const std::string& file_path) {
    // Открываем файл
    int file_fd = open(file_path.c_str(), O_RDONLY);
    if (file_fd == -1) {
        perror("open");
        return false;
    }

    // Получаем размер файла
    struct stat file_stat;
    if (fstat(file_fd, &file_stat) == -1) {
        perror("fstat");
        close(file_fd);
        return false;
    }
    off_t file_size = file_stat.st_size;
    off_t offset = 0;          // текущая позиция в файле
    ssize_t sent_total = 0;    // всего отправлено байт

    // Для измерения скорости (опционально)
    auto start_time = std::chrono::steady_clock::now();

    while (sent_total < file_size) {
        // Определяем, сколько байт попытаться отправить за раз
        // Можно выбрать фиксированный размер, например 64 КБ
        size_t count = 64 * 1024; // 64 KB
        // Но не выходим за границы файла
        if (offset + count > file_size) {
            count = file_size - offset;
        }

        // Вызов sendfile
        ssize_t sent = sendfile(socket_fd, file_fd, &offset, count);
        if (sent == -1) {
            // Обработка ошибок (EAGAIN/EWOULDBLOCK для неблокирующего сокета и т.д.)
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                // Для неблокирующего режима можно подождать или использовать poll/select
                // В данном примере для простоты считаем это ошибкой
                perror("sendfile (non-blocking?)");
                close(file_fd);
                return false;
            } else if (errno == EINTR) {
                // Прервано сигналом — продолжаем
                continue;
            } else {
                perror("sendfile");
                close(file_fd);
                return false;
            }
        }

        // Обновляем прогресс
        sent_total += sent;
        // offset автоматически обновлён (sendfile изменяет его, если передан указатель)
        // Вычисляем процент выполнения
        int percent = static_cast<int>((static_cast<double>(sent_total) / file_size) * 100);
         //prstd::cout "\rПрогресс: " << percent << "% (" << sent_total << "/" << file_size << " байт)" << std::flush;

        // Можно также рассчитать скорость и оставшееся время
        auto now = std::chrono::steady_clock::now();
        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - start_time).count();
        if (elapsed > 0) {
            double speed = static_cast<double>(sent_total) / elapsed * 1000; // байт/с
            // ...
        }
    }

     //prstd::cout << "Файл успешно отправлен." << std::endl;
    close(file_fd);
    return true;
}