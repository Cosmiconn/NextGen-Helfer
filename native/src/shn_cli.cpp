#include "shn_reader.hpp"
#include <filesystem>
#include <fstream>
#include <iostream>
#include <stdexcept>
int main(int argc,char** argv){
  if(argc!=4){std::cerr<<"Usage: IsyaShnInspect <Produce.shn|Production.shn> <output.csv> <schema.txt>\n";return 2;}
  try{
    auto table=isya::shn::read(argv[1]);
    std::ofstream csv(argv[2],std::ios::binary|std::ios::trunc),schema(argv[3],std::ios::binary|std::ios::trunc);
    if(!csv||!schema)throw std::runtime_error("Ausgabedateien nicht schreibbar");
    csv<<"\xef\xbb\xbf"<<isya::shn::csv(table);
    schema<<isya::shn::profile(table);
    std::cout<<"OK: "<<table.rows.size()<<" records, "<<table.columns.size()<<" columns\n";
  }catch(const std::exception& ex){std::cerr<<"SHN Import fehlgeschlagen: "<<ex.what()<<"\n";return 1;}
  return 0;
}
