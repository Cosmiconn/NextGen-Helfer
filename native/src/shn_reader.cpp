#include "shn_reader.hpp"
#include <algorithm>
#include <bit>
#include <cmath>
#include <cstring>
#include <fstream>
#include <iomanip>
#include <limits>
#include <sstream>
#include <stdexcept>
#include <string_view>
namespace isya::shn { namespace {
using byte=std::uint8_t;
std::string utf8latin(const std::string& src) {
  std::string result;
  for(unsigned char c:src) {
    if(c < 128)result += static_cast<char>(c);
    else {result += static_cast<char>(0xc0|(c>>6));result += static_cast<char>(0x80|(c&63));}
  }
  return result;
}
std::string clean(const byte* p,std::size_t n) {
  auto end=std::find(p,p+n,byte{0});
  return utf8latin(std::string(reinterpret_cast<const char*>(p),reinterpret_cast<const char*>(end)));
}
class Reader {
  const std::vector<byte>& bytes_;std::size_t pos_=0;
public:
  explicit Reader(const std::vector<byte>& bytes):bytes_(bytes){}
  std::size_t position()const{return pos_;}
  void require(std::size_t n)const{if(n>bytes_.size()-pos_)throw std::runtime_error("SHN ist abgeschnitten oder hat ein unerwartetes Tabellenlayout");}
  std::uint8_t u8(){require(1);return bytes_[pos_++];}
  std::uint16_t u16(){require(2);auto v=std::uint16_t(bytes_[pos_])|(std::uint16_t(bytes_[pos_+1])<<8);pos_+=2;return v;}
  std::uint32_t u32(){require(4);auto v=std::uint32_t(bytes_[pos_])|(std::uint32_t(bytes_[pos_+1])<<8)|(std::uint32_t(bytes_[pos_+2])<<16)|(std::uint32_t(bytes_[pos_+3])<<24);pos_+=4;return v;}
  std::int16_t i16(){return static_cast<std::int16_t>(u16());}
  std::int32_t i32(){return static_cast<std::int32_t>(u32());}
  float f32(){return std::bit_cast<float>(u32());}
  std::string fixed(std::size_t n){require(n);std::string s=clean(bytes_.data()+pos_,n);pos_+=n;return s;}
  std::string zstr(){std::string b;for(int i=0;i<16384;++i){auto c=u8();if(!c)return utf8latin(b);b+=char(c);}throw std::runtime_error("Ungueltige variable Textspalte");}
};
std::string field(Reader& r,const Column& c){
  if(c.length<0||c.length>65536)throw std::runtime_error("Unplausible Spaltenlaenge");
  switch(c.type){
    case 1:case 12:case 16:return std::to_string(r.u8());
    case 2:return std::to_string(r.u16());
    case 3:case 11:case 18:case 27:return std::to_string(r.u32());
    case 5:{float v=r.f32();if(!std::isfinite(v))return "";std::ostringstream o;o<<std::setprecision(9)<<v;return o.str();}
    case 13:case 21:return std::to_string(r.i16());
    case 20:return std::to_string(static_cast<std::int8_t>(r.u8()));
    case 22:return std::to_string(r.i32());
    case 9:case 24:return r.fixed(static_cast<size_t>(c.length));
    case 26:return r.zstr();
    default:throw std::runtime_error("Unbekannter SHN-Spaltentyp "+std::to_string(c.type)+" bei "+c.name+". Unveraendert belassen und Layout pruefen.");
  }
}
std::string quote(std::string_view x){std::string s="\"";for(char c:x){if(c=='\"')s+="\"\"";else if(c=='\r'||c=='\n')s+=' ';else s+=c;}return s+'\"';}
}
Table read(const std::filesystem::path& input) {
  std::ifstream f(input,std::ios::binary|std::ios::ate);
  if(!f)throw std::runtime_error("Datei nicht lesbar: "+input.string());
  auto length=f.tellg();if(length<16||length>64*1024*1024)throw std::runtime_error("SHN muss zwischen 16 Byte und 64 MiB gross sein");
  f.seekg(0);std::vector<byte> raw(static_cast<size_t>(length));
  if(!f.read(reinterpret_cast<char*>(raw.data()),length))throw std::runtime_error("SHN-Lesefehler");
  // Normal .shn: 32-byte container header, LE total size at +32, encrypted payload from +36.
  // Also accept plaintext/decrypted .dec for inspection; detect format from extension.
  std::vector<byte> data;
  if(input.extension()==".shn" || input.extension()==".SHN"){
    if(raw.size()<52)throw std::runtime_error("SHN-Header zu kurz");
    std::uint32_t file_size=std::uint32_t(raw[32])|(std::uint32_t(raw[33])<<8)|(std::uint32_t(raw[34])<<16)|(std::uint32_t(raw[35])<<24);
    if(file_size!=raw.size())throw std::runtime_error("SHN-Längenfeld ungueltig: andere Formatversion? Bitte Dateikopie pruefen");
    data.assign(raw.begin()+36,raw.end());
    std::uint8_t key=static_cast<byte>(data.size());
    for(std::size_t index=data.size();index-- > 0;){
      data[index]^=key;
      auto i=static_cast<byte>(index);
      key=static_cast<byte>(static_cast<byte>((i&15)+0x55) ^ static_cast<byte>(i*11) ^ key ^ 0xaa);
    }
  }else data=std::move(raw);
  Reader r(data);Table table;
  table.header=r.u32();auto count=r.u32();table.record_length=r.u32();auto columns=r.u32();
  if(count>500000||columns<1||columns>400||table.record_length<2||table.record_length>65536)
    throw std::runtime_error("Unplausibler SHN-Tabellenkopf (Format nicht erkannt)");
  table.columns.reserve(columns);
  std::uint64_t fixed_length=2;
  for(std::uint32_t i=0;i<columns;++i){
    auto name=r.fixed(48);auto type=r.u32();auto width=r.i32();
    if(name.empty())name="UnkCol"+std::to_string(i);
    if(width<0||width>65536)throw std::runtime_error("Unplausible Spaltenbreite");
    if(type!=26)fixed_length+=static_cast<std::uint32_t>(width);
    table.columns.push_back({name,type,width});
  }
  if(fixed_length != table.record_length)throw std::runtime_error("SHN-Zeilenlaenge passt nicht zum Spaltenlayout; Quelldatei pruefen");
  table.rows.reserve(count);
  for(std::uint32_t i=0;i<count;++i){
    auto start=r.position();auto declared=r.u16();
    std::vector<std::string> row;row.reserve(columns);
    for(const auto& c:table.columns)row.push_back(field(r,c));
    if(declared!=0 && declared!=r.position()-start)
      throw std::runtime_error("SHN-Zeilenmarker unerwartet in Zeile "+std::to_string(i));
    table.rows.push_back(std::move(row));
  }
  return table;
}
std::string csv(const Table& t){std::ostringstream out;for(size_t i=0;i<t.columns.size();++i){if(i)out<<',';out<<quote(t.columns[i].name);}out<<'\n';for(const auto& row:t.rows){for(size_t j=0;j<row.size();++j){if(j)out<<',';out<<quote(row[j]);}out<<'\n';}return out.str();}
std::string profile(const Table& t){std::ostringstream out;out<<"SHN Header: "<<t.header<<"\nRows: "<<t.rows.size()<<"\nRecordLength: "<<t.record_length<<"\nColumns:\n";for(size_t i=0;i<t.columns.size();++i)out<<i<<"\t"<<t.columns[i].name<<"\ttype="<<t.columns[i].type<<"\tlen="<<t.columns[i].length<<"\n";return out.str();}
}
