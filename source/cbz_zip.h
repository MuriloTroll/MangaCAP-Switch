#pragma once
#include <cstdint>
#include <string>
#include <vector>
struct CbzEntry { std::string name; uint32_t compressed,uncompressed,offset,crc; uint16_t method,flags; };
class CbzZip {
    std::string path_;
    std::vector<CbzEntry> entries_;
    bool valid_=false;
    bool rar_=false;
public:
    explicit CbzZip(const std::string& path);
    bool valid() const {return valid_;}
    const std::vector<CbzEntry>& entries() const {return entries_;}
    bool read(const std::string& name,std::vector<unsigned char>& data,size_t limit) const;
};
