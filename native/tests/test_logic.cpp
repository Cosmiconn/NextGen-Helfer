#include "logic.hpp"
#include <stdexcept>
static void require(bool ok) { if(!ok) throw std::runtime_error("Test assertion failed"); }
#include <iostream>
int main(){
  using namespace isya;
  require(normalize("Raues  Leder")=="raues leder");
  require(normalize("KrÄUtER")=="krauter");
  require(parse_name("HP 120\nRaues Leder\nLevel 60")=="Raues Leder");
  require(parse_price("Preis: 1g 250s 30k")==1'250'030);
  require(parse_price("2 Gem") == 200'000'000);
  require(parse_price("0,25g") == 250'000);
  require(!parse_price("1.000g"));
  require(!parse_price("Level 60\nSTR +25"));
  require(parse_quantity("Menge: 25")==25);
  require(!parse_quantity("Level 25"));
  Sample s; s.name="Raues Leder";s.price=1'500'000;s.quantity=3;s.quantity_assumed=false;
  std::vector<Target> t={{"Raues Leder",24,500'000,"Rollen"}};
  auto m=match_sample(s,t);require(m.status=="affordable"&&*m.unit==500'000);
  s.price=2'000'000;require(match_sample(s,t).status=="expensive");
  s.price.reset();require(match_sample(s,t).status=="needed");
  s.name="Raues Ledre";require(match_sample(s,t).status!="affordable");
  s.name="Nicht auf Liste";require(match_sample(s,t).status=="none");
  std::cout<<"C++ core logic tests: OK\n";
}
