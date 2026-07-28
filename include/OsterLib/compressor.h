#pragma once
#include <zip.h>
#include <filesystem>
#include <iostream>

namespace fs = std::filesystem;

namespace cpp::zip_ns{
    class Compressor{
        enum ZIP_FLAGS{
            //Perform additional stricter consistency checks on the archive, and error if they fail.
            CHECK_CONSISTENCY = ZIP_CHECKCONS,
            //Create the archive if it does not exist.
            CREATE = ZIP_CREATE,
            //Error if archive already exists.
            CHECK_EXISTS = ZIP_EXCL,
            //If archive exists, ignore its current contents. In other words, handle it the same way as an empty archive.
            TRUNCATE = ZIP_TRUNCATE,
            //Open archive in read-only mode.
            READ_ONLY = ZIP_RDONLY
        };

        Compressor(const fs::path& zip_path, ZIP_FLAGS flags):path_(zip_path){
            int error_code = 0;
            zip_ = zip_open(zip_path.c_str(),flags,&error_code);
            if(!zip_){
                zip_error error_at_open;
                zip_error_init_with_code(&error_at_open,error_code);
                throw std::runtime_error(zip_error_strerror(&error_at_open));
                return;
            }
        }

        Compressor(const fs::path& zip_dir, const std::string& zip_name, ZIP_FLAGS flags):path_(zip_dir/zip_name){
            int error_code = 0;
            zip_ = zip_open(path_.c_str(),flags,&error_code);
            if(!zip_){
                zip_error error_at_open;
                zip_error_init_with_code(&error_at_open,error_code);
                throw std::runtime_error(zip_error_strerror(&error_at_open));
                return;
            }
        }
        public:
        static Compressor create_archive(const fs::path& zip_dir, const std::string& zip_name){
            return Compressor(zip_dir,zip_name,static_cast<ZIP_FLAGS>(ZIP_FLAGS::CREATE|ZIP_FLAGS::TRUNCATE));
        }
        static Compressor create_archive(const fs::path& zip_path){
            return Compressor(zip_path,static_cast<ZIP_FLAGS>(ZIP_FLAGS::CREATE|ZIP_FLAGS::TRUNCATE));
        }

        static Compressor open_archive(const fs::path& zip_dir, const std::string& zip_name){
            return Compressor(zip_dir,zip_name,static_cast<ZIP_FLAGS>(0));
        }
        static Compressor open_archive(const fs::path& zip_path){
            return Compressor(zip_path,static_cast<ZIP_FLAGS>(0));
        }

        static Compressor open_archive_read_only(const fs::path& zip_dir, const std::string& zip_name){
            return Compressor(zip_dir,zip_name,static_cast<ZIP_FLAGS>(ZIP_FLAGS::READ_ONLY));
        }
        static Compressor open_archive_read_only(const fs::path& zip_path){
            return Compressor(zip_path,static_cast<ZIP_FLAGS>(ZIP_FLAGS::READ_ONLY));
        }

        bool add_file(const fs::path& rel_path, const fs::path& file_path) noexcept{
            if(zip_!=NULL){
                zip_source* zs = zip_source_file_create(file_path.c_str(),0,0,&err_);
                if(!zs){
                    //std::cout<<zip_error_strerror(&err_)<<std::endl;
                    return false;
                }
                if(zip_file_add(zip_,fs::relative(file_path,rel_path).c_str(), zs,ZIP_FL_ENC_UTF_8)<0){
                    //std::cout<<zip_error_strerror(&err_)<<std::endl;
                    zip_source_free(zs);
                    return false;
                }
                else return true;
            }
            else return false;
        }

        static bool add_file(const fs::path& archive_path,const fs::path& rel_path, const fs::path& file_path) noexcept{
            try{
                return open_archive(archive_path).add_file(rel_path,file_path);
            }
            catch(const std::exception& err){
                return false;
            }
        }
        
        const fs::path& get_zip_path() const{
            return path_;
        }

        ~Compressor(){
            if(zip_)
                zip_close(zip_);
        }

        private:
        const fs::path& path_;
        zip* zip_ = nullptr;
        zip_error_t err_;
    };
}
