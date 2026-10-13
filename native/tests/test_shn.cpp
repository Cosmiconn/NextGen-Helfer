#include "shn_reader.hpp"
#include "world_match.hpp"
#include <stdexcept>
static void require(bool ok) { if(!ok) throw std::runtime_error("Test assertion failed"); }
#include <filesystem>
#include <fstream>
#include <iostream>
#include <vector>
using namespace isya;
int main(){
  std::vector<Target> targets={{"Raues Leder",20,std::nullopt,"Rohstoff"},{"Silbererz",15,std::nullopt,"Bergbau"}};
  auto hits=find_visible_targets({{"Raues Leder",20,12,150,25,95},{"Silbererz",200,40,80,22,35},{"Gold",0,0,80,20,99}},targets,300,400);
  require(hits.size()==1 && hits[0].name=="Raues Leder" && hits[0].x==320 && hits[0].y==412);
  auto ambiguous=find_visible_targets({{"Raues Ledre",20,12,150,25,99}},targets,0,0);
  require(ambiguous.empty());
  std::vector<std::uint8_t> raw(36,0);
  auto put32=[&](std::uint32_t x){for(int i=0;i<4;i++)raw.push_back(static_cast<std::uint8_t>(x>>(i*8)));};
  auto put16=[&](std::uint16_t x){raw.push_back(static_cast<std::uint8_t>(x));raw.push_back(static_cast<std::uint8_t>(x>>8));};
  put32(1);put32(1);put32(2+4+13);put32(2);
  auto col=[&](const std::string& name,std::uint32_t type,std::uint32_t len){for(int i=0;i<48;i++)raw.push_back(i<static_cast<int>(name.size())?name[i]:0);put32(type);put32(len);};
  col("ID",3,4);col("RecipeName",9,13);
  put16(19);put32(52);for(char c:std::string("Bronze Hammer"))raw.push_back(c);while(raw.size()<36+16+2*56+19)raw.push_back(0);
  auto dec=raw;dec.erase(dec.begin(),dec.begin()+36);
  std::uint8_t key=static_cast<std::uint8_t>(dec.size());
  for(std::size_t i=dec.size();i-- >0;){dec[i]^=key;auto b=static_cast<std::uint8_t>(i);key=static_cast<std::uint8_t>(static_cast<std::uint8_t>((b&15)+0x55)^static_cast<std::uint8_t>(b*11)^key^0xaa);}
  // Cipher operates on the original bytes backward, hence inverse direction differs.
  // Instead decode the synthetic payload by reversing the XOR chain (same stream state independent of plaintext).
  std::vector<std::uint8_t> cipher=raw;cipher.resize(36);cipher.insert(cipher.end(),dec.begin(),dec.end());
  auto size=static_cast<std::uint32_t>(cipher.size());for(int i=0;i<4;i++)cipher[32+i]=static_cast<std::uint8_t>(size>>(i*8));
  auto p=std::filesystem::temp_directory_path()/"isya_synthetic_shn_test.shn";
  {std::ofstream f(p,std::ios::binary);f.write(reinterpret_cast<const char*>(cipher.data()),static_cast<std::streamsize>(cipher.size()));}
  auto t=isya::shn::read(p);std::filesystem::remove(p);
  require(t.rows.size()==1 && t.columns.size()==2 && t.rows[0][0]=="52");
  require(t.rows[0][1]=="Bronze Hammer");
  auto c=isya::shn::csv(t);require(c.find("Bronze Hammer")!=std::string::npos);
  std::cout<<"World matching + SHN parsing: OK\n";
}
