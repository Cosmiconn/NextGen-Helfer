// Offline Fiesta-SHN importer: user-selected files only.
#include "shn_reader.hpp"
#include <charconv>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <map>
#include <regex>
#include <set>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>
using isya::shn::Table;
namespace fs=std::filesystem;
namespace {
using Row=std::vector<std::string>;
std::map<std::string,size_t> cols(const Table& t) {
 std::map<std::string,size_t> c;
 for(size_t j=0;j<t.columns.size();++j)
  if(!c.emplace(t.columns[j].name,j).second)throw std::runtime_error("Duplicate SHN column");
 return c;
}
std::string get(const Row& r,const std::map<std::string,size_t>& c,const std::string& name){
 auto at=c.find(name);
 if(at==c.end()||at->second>=r.size())throw std::runtime_error("Missing SHN field: "+name);
 return r[at->second];
}
int number(const std::string& s,bool nonzero=false){
 int x=0;auto [p,e]=std::from_chars(s.data(),s.data()+s.size(),x);
 if(e!=std::errc{}||p!=s.data()+s.size()||x<0||(nonzero&&x==0))throw std::runtime_error("Invalid SHN number: "+s);
 return x;
}
std::string quoted(const std::string& s) {
 std::string out="\"";
 for(char c:s){if(c=='\"')out+="\"\"";else if(c=='\r'||c=='\n')out+=' ';else out+=c;}
 return out+'\"';
}
void row(std::ostream& out,std::initializer_list<std::string> cells) {
 bool first=true;for(const auto& c:cells){if(!first)out<<';';first=false;out<<quoted(c);}out<<'\n';
}
int tier(const std::string& name) {
 static const std::regex pattern(R"(\(Tier ([0-9]+)\)|\[([0-9]+)\])");
 std::smatch m;
 if(!std::regex_search(name,m,pattern))return 0;
 return number(m[1].matched?m[1].str():m[2].str());
}
std::string category(const std::string& name,const std::string& product) {
 if(name.starts_with("Material Composition"))return "Materialaufwertung";
 if(name.starts_with("Material Decomposition"))return "Materialzerlegung";
 if(name.starts_with("Scroll"))return "Rolle";
 if(name.starts_with("Alchemy")) {
  if(product.starts_with("Elrue [")||product.starts_with("Lix [")||product.starts_with("Xir ["))return "Stein";
  return "Trank";
 }
 return "Unbekannt";
}
std::ofstream output(const fs::path& p) {
 std::ofstream f(p,std::ios::binary|std::ios::trunc);
 if(!f)throw std::runtime_error("Cannot write "+p.string());
 f<<"\xef\xbb\xbf";return f;
}
}
int main(int argc,char** argv) {
 if(argc!=5){std::cerr<<"Usage: IsyaShnCatalog Produce.shn ItemInfo.shn Gather.shn OutputDirectory\n";return 2;}
 try {
  Table p=isya::shn::read(argv[1]), i=isya::shn::read(argv[2]), g=isya::shn::read(argv[3]);
  auto pc=cols(p),ic=cols(i),gc=cols(g);
  if(p.rows.empty()||i.rows.empty()||g.rows.empty())throw std::runtime_error("Empty SHN table");
  for(auto key:{"ProductID","ProduceIndex","Name","Product","Lot","MasteryGain","NeededMasteryGain"})(void)get(p.rows[0],pc,key);
  for(auto key:{"ID","InxName","Name"})(void)get(i.rows[0],ic,key);
  for(auto key:{"GatherID","Index","Type","NeededTool0","EqipItemView","AniNumber","Gauge"})(void)get(g.rows[0],gc,key);
  using Item=std::pair<std::string,std::string>;
  std::map<std::string,std::vector<Item>> lookup;
  for(const auto& r:i.rows){
   auto name=get(r,ic,"InxName");
   if(!name.empty()&&name!="-")lookup[name].push_back({get(r,ic,"ID"),get(r,ic,"Name")});
  }
  auto resolve=[&](const std::string& key)->Item {
   auto at=lookup.find(key);
   if(at==lookup.end())throw std::runtime_error("Unknown item index "+key);
   if(at->second.size()!=1)throw std::runtime_error("Ambiguous item index "+key);
   return at->second.front();
  };
  fs::path dir(argv[4]);fs::create_directories(dir);
  auto master=output(dir/"recipes-master-2016.csv");
  auto app=output(dir/"IsyaGold-Import-EN2016.csv");
  auto gather=output(dir/"gather-actions-2016.csv");
  row(master,{"RecipeID","RecipeIndex","RecipeName","Category","Tier","ProductIndex","ProductItemID","ProductName","OutputQty","MasteryGain","NeededMasteryGain","IngredientIndex","IngredientItemID","IngredientName","Quantity"});
  row(app,{"Kategorie","Tier","Produktname","Zutat","Menge","Quelle","Status"});
  row(gather,{"GatherID","Action","Type","RequiredTool","ViewItem","Animation","Gauge"});
  std::set<std::string> seen,rawNames; std::map<std::string,int> counts;
  int ingredientLinks=0,unknownTiers=0;
  for(const auto& r:p.rows) {
   auto id=get(r,pc,"ProductID");
   if(!seen.insert(id).second)throw std::runtime_error("Duplicate recipe "+id);
   auto desc=get(r,pc,"Name");
   auto key=get(r,pc,"Product");
   auto prod=resolve(key);
   auto cat=category(desc,prod.second);
   int lv=tier(desc);if(!lv)lv=tier(prod.second);
   if(lv==0)++unknownTiers; counts[cat]++;
   auto lot=std::to_string(number(get(r,pc,"Lot"),true));
   auto mastery=std::to_string(number(get(r,pc,"MasteryGain")));
   auto needed=std::to_string(number(get(r,pc,"NeededMasteryGain")));
   int actual=0;
   for(int k=0;k<8;++k) {
    auto ingredient=get(r,pc,"Raw"+std::to_string(k));
    auto count=number(get(r,pc,"Quantity"+std::to_string(k)));
    if(ingredient=="-"||ingredient.empty()){
     if(count)throw std::runtime_error("Quantity without ingredient in "+id);
     continue;
    }
    if(!count)throw std::runtime_error("Ingredient without quantity in "+id);
    auto ing=resolve(ingredient);
    ++actual;++ingredientLinks;rawNames.insert(ingredient);
    row(master,{id,get(r,pc,"ProduceIndex"),desc,cat,std::to_string(lv),key,prod.first,prod.second,lot,mastery,needed,ingredient,ing.first,ing.second,std::to_string(count)});
    row(app,{cat,std::to_string(lv),prod.second,ing.second,std::to_string(count),"","EN 2016 SHN - Testdaten, nicht DE aktuell"});
   }
   if(!actual)throw std::runtime_error("Recipe without ingredients "+id);
  }
  for(const auto& r:g.rows) row(gather,{get(r,gc,"GatherID"),get(r,gc,"Index"),get(r,gc,"Type"),get(r,gc,"NeededTool0"),get(r,gc,"EqipItemView"),get(r,gc,"AniNumber"),get(r,gc,"Gauge")});
  master.flush();app.flush();gather.flush();
  if(!master||!app||!gather)throw std::runtime_error("CSV write error");
  std::ostringstream report;
  report<<"Version: EN 2016 SHN (NICHT aktuelle deutsche Version)\n";
  report<<"ItemInfo: "<<i.rows.size()<<"\nRezepte: "<<p.rows.size()<<"\n";
  report<<"Zutaten-Verknuepfungen: "<<ingredientLinks<<"\nEindeutige Zutatenschluessel: "<<rawNames.size()<<"\n";
  report<<"Ohne ausgewiesenen Tier: "<<unknownTiers<<"\nGather-Aktionen (KEINE Spawnkoordinaten): "<<g.rows.size()<<"\n";
  for(const auto& [cat,n]:counts)report<<"Kategorie "<<cat<<": "<<n<<"\n";
  auto out=output(dir/"import-report-2016.txt");out<<report.str();
  std::cout<<report.str();return 0;
 }catch(const std::exception& e){std::cerr<<"SHN catalogue failed: "<<e.what()<<"\n";return 1;}
}
