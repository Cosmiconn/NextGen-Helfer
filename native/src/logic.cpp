#include "logic.hpp"
#include <algorithm>
#include <cctype>
#include <cmath>
#include <limits>
#include <regex>
#include <sstream>
#include <utility>

namespace isya {
namespace {
std::string lower(std::string text) {
  for (char& c : text) if (static_cast<unsigned char>(c)<128) c=static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
  return text;
}
void replace_all(std::string& s,const std::string& from,const std::string& to) {
  size_t p=0; while ((p=s.find(from,p))!=std::string::npos) {s.replace(p,from.size(),to); p+=to.size();}
}
std::string trim(std::string s) {
  size_t a=s.find_first_not_of(" \t\r\n\xEF\xBB\xBF");
  if(a==std::string::npos) return {};
  size_t z=s.find_last_not_of(" \t\r\n");return s.substr(a,z+1-a);
}
size_t edit_distance(const std::string& a,const std::string& b) {
  std::vector<size_t> prev(b.size()+1),cur(b.size()+1);
  for(size_t j=0;j<=b.size();++j)prev[j]=j;
  for(size_t i=0;i<a.size();++i) {
    cur[0]=i+1;
    for(size_t j=0;j<b.size();++j) {
      cur[j+1]=std::min({cur[j]+1,prev[j+1]+1,prev[j]+(a[i]!=b[j])});
    }
    std::swap(cur,prev);
  }
  return prev.back();
}
double similarity(const std::string& a,const std::string& b) {
  auto n=std::max(a.size(),b.size());return n?1.0-double(edit_distance(a,b))/double(n):1.0;
}
}
std::string normalize(std::string_view input) {
  std::string s=lower(std::string(input));
  for(auto [from,to]:{std::pair{"ä","a"}, {"ö","o"},{"ü","u"},{"Ä","a"},{"Ö","o"},{"Ü","u"},{"ß","ss"}})replace_all(s,from,to);
  std::string out;bool space=false;
  for(unsigned char c:s){if(std::isalnum(c)&&c<128){out+=char(c);space=false;}else if(!space&&!out.empty()){out+=' ';space=true;}}
  if(!out.empty()&&out.back()==' ')out.pop_back();
  return out;
}
std::string parse_name(std::string_view raw) {
  std::istringstream lines{std::string(raw)};std::string line;int counter=0;
  const std::vector<std::string> excluded={"shop","gegenstand","erforderlich","benotigt","preis","verkauf","kaufen","schaden","def","ausdauer","starke","intelligenz","krit","chance","str","spr","mp","hp","level","lvl","bindung","beschreibung","qualitat"};
  while(std::getline(lines,line) && counter++<7){
    line=trim(line);if(line.size()<3||line.size()>100)continue;
    for(char& c:line)if(c=='|'||c=='\t')c=' ';
    const auto value=normalize(line);if(value.empty())continue;
    bool contains_letters=false;for(unsigned char c:line){if(std::isalpha(c)||c>=128){contains_letters=true;break;}}if(!contains_letters)continue;
    bool skip=false;for(const auto& s:excluded)if(value==s || value.rfind(s+" ",0)==0){skip=true;break;}
    if(skip)continue;
    while(!line.empty() && (line.front()==':'||line.front()=='.'||line.front()=='-'||line.front()=='*'))line.erase(line.begin());
    while(!line.empty() && (line.back()==':'||line.back()=='.'||line.back()=='-'||line.back()=='*'))line.pop_back();
    return trim(line);
  }
  return {};
}
std::optional<std::int64_t> parse_price(std::string_view text) {
  // Reject ambiguous OCR thousands notation (e.g. "1.000g").
  std::regex ambiguous(R"(\b\d{1,3}[.,]\d{3}\s*(?:gem|gems|gold|silber|kupfer|[gsk])\b)",std::regex_constants::icase);
  std::string raw(text);if(std::regex_search(raw,ambiguous))return std::nullopt;
  std::vector<std::string> lines;std::istringstream stream(raw);std::string line;
  while(std::getline(stream,line))lines.push_back(line);
  std::stable_sort(lines.begin(),lines.end(),[](const auto& a,const auto& b){
    std::regex label(R"(preis|gesamt|kosten)",std::regex_constants::icase);
    return std::regex_search(a,label)&&!std::regex_search(b,label);
  });
  const std::regex chunks(R"((\d{1,12}(?:[.,]\d{1,3})?)\s*(gem|gems|gold|silber|kupfer|[gsk])\b)",std::regex_constants::icase);
  for(const auto& row:lines){std::int64_t total=0;bool any=false;
    for(std::sregex_iterator it(row.begin(),row.end(),chunks),end;it!=end;++it){
      auto match=*it;auto number=match.str(1);std::replace(number.begin(),number.end(),',','.');
      double val=0;try{val=std::stod(number);}catch(...){return std::nullopt;}
      std::string cur=lower(match.str(2));
      std::int64_t mult=(cur=="gem"||cur=="gems")?100'000'000:(cur=="g"||cur=="gold")?1'000'000:(cur=="s"||cur=="silber")?1000:1;
      if(val<=0||val>1e10 || val*mult>9e17)return std::nullopt;
      auto part=static_cast<std::int64_t>(std::llround(val*mult));
      if(total>900'000'000'000'000'000LL-part)return std::nullopt;
      total+=part;any=true;
    }
    if(any && total>0)return total;
  }
  return std::nullopt;
}
std::optional<std::int64_t> parse_quantity(std::string_view raw){
  static const std::regex quantity(R"(\b(?:anzahl|menge|stack|stk|stuck|stueck|x)\s*[:x]?\s*(\d{1,8})\b)",std::regex_constants::icase);
  const auto s=std::string(raw);std::smatch m;if(!std::regex_search(s,m,quantity))return std::nullopt;
  try{auto v=std::stoll(m[1].str());if(v>0&&v<=100'000'000)return v;}catch(...){}
  return std::nullopt;
}
Match match_sample(const Sample& sample,const std::vector<Target>& targets){
  Match result;auto key=normalize(sample.name);if(key.empty())return result;
  const Target* exact=nullptr;const Target* guess=nullptr;double highest=0,runner=0;
  for(const auto& item:targets){auto candidate=normalize(item.name);
    if(candidate==key){exact=&item;break;}
    if(key.size()>=8){auto sim=similarity(key,candidate);if(sim>highest){runner=highest;highest=sim;guess=&item;}else if(sim>runner)runner=sim;}
  }
  const Target* chosen=exact?exact:((highest>=.90&&highest-runner>.06)?guess:nullptr);
  if(!chosen)return result;
  result.name=chosen->name;result.need=chosen->need;result.max_unit=chosen->max_unit;
  if(!exact){result.status="review";result.message="Aehnlicher OCR-Name: manuell pruefen";return result;}
  result.qty_assumed=sample.quantity_assumed;
  if(sample.price && sample.quantity>0)result.unit=static_cast<std::int64_t>(std::llround(double(*sample.price)/double(sample.quantity)));
  result.status="needed";result.message="Benoetigtes Material erkannt";
  if(chosen->max_unit && result.unit){bool affordable= *result.unit<=*chosen->max_unit;
    result.status=affordable?"affordable":"expensive";
    result.message=affordable?"Im Preislimit (OCR ungeprueft)":"Ueber dem Preislimit (OCR ungeprueft)";
  }
  return result;
}
std::string signature(const Sample& s){return normalize(s.name)+"|"+(s.price?std::to_string(*s.price):"")+"|"+std::to_string(s.quantity);}
}
