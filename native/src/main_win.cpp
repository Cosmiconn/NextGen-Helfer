#ifdef _WIN32
#include "logic.hpp"
#include "world_match.hpp"
#include <Windows.h>
#include <Shellapi.h>
#include <nlohmann/json.hpp>
#include <httplib.h>
#include <tesseract/baseapi.h>
#include <tesseract/resultiterator.h>
#include <algorithm>
#include <atomic>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <ctime>
#include <stdexcept>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <memory>
#include <mutex>
#include <optional>
#include <regex>
#include <set>
#include <sstream>
#include <string>
#include <thread>
#include <vector>

using Json=nlohmann::json;
using namespace std::chrono_literals;
namespace fs=std::filesystem;
namespace {
constexpr int kPort=8765;
fs::path app_dir(){wchar_t buf[32768]{};auto count=GetModuleFileNameW(nullptr,buf,32768);return fs::path(std::wstring(buf,count)).parent_path();}
std::string read_file(const fs::path& p){std::ifstream f(p,std::ios::binary);if(!f)return {};return {std::istreambuf_iterator<char>(f),{}};}
void write_json(const fs::path& p,const Json& j){std::ofstream f(p,std::ios::binary|std::ios::trunc);if(!f)throw std::runtime_error("Konfigurationsdatei nicht beschreibbar");f<<j.dump(2,' ',false,Json::error_handler_t::replace);}
std::wstring widen(const std::string& s){if(s.empty())return {};int n=MultiByteToWideChar(CP_UTF8,0,s.c_str(),-1,nullptr,0);if(n<=0)return L"";std::wstring w(n,0);MultiByteToWideChar(CP_UTF8,0,s.c_str(),-1,w.data(),n);w.pop_back();return w;}
std::string utf8(const std::wstring& s){if(s.empty())return {};int n=WideCharToMultiByte(CP_UTF8,0,s.c_str(),-1,nullptr,0,nullptr,nullptr);std::string out(n,0);WideCharToMultiByte(CP_UTF8,0,s.c_str(),-1,out.data(),n,nullptr,nullptr);out.pop_back();return out;}
std::string base64(const std::vector<unsigned char>& bytes){static constexpr char abc[]="ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";std::string output;output.reserve((bytes.size()+2)/3*4);for(size_t i=0;i<bytes.size();i+=3){unsigned a=bytes[i],b=i+1<bytes.size()?bytes[i+1]:0,c=i+2<bytes.size()?bytes[i+2]:0;output.push_back(abc[a>>2]);output.push_back(abc[((a&3)<<4)|(b>>4)]);output.push_back(i+1<bytes.size()?abc[((b&15)<<2)|(c>>6)]:'=');output.push_back(i+2<bytes.size()?abc[c&63]:'=');}return output;}
std::string now_iso(){auto t=std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());std::tm tm{};gmtime_s(&tm,&t);char out[40];std::strftime(out,sizeof(out),"%Y-%m-%dT%H:%M:%SZ",&tm);return out;}
std::int64_t positive_integer(const Json& v,std::int64_t min,std::int64_t max){if(!v.is_number_integer())throw std::runtime_error("Ganzzahl erwartet");auto n=v.get<std::int64_t>();if(n<min||n>max)throw std::runtime_error("Wert ausserhalb des erlaubten Bereichs");return n;}
struct Rect {int dx=0,dy=0,w=0,h=0;};
Json world_rect_json(const std::optional<Rect>& r){if(!r)return nullptr;return Json{{"x",r->dx},{"y",r->dy},{"w",r->w},{"h",r->h}};}
std::optional<Rect> parse_world_rect(const Json& j){if(j.is_null())return std::nullopt;if(!j.is_object())throw std::runtime_error("Weltbereich muss ein Objekt sein");Rect r;r.dx=int(positive_integer(j.at("x"),-16000,16000));r.dy=int(positive_integer(j.at("y"),-16000,16000));r.w=int(positive_integer(j.at("w"),20,3500));r.h=int(positive_integer(j.at("h"),20,3500));return r;}
Json rect_json(const std::optional<Rect>& r){if(!r)return nullptr;return Json{{"dx",r->dx},{"dy",r->dy},{"w",r->w},{"h",r->h}};}
std::optional<Rect> parse_rect(const Json& j){if(j.is_null())return std::nullopt;if(!j.is_object())throw std::runtime_error("Region muss ein Objekt sein");Rect r;r.dx=int(positive_integer(j.at("dx"),-16000,16000));r.dy=int(positive_integer(j.at("dy"),-16000,16000));r.w=int(positive_integer(j.at("w"),8,3500));r.h=int(positive_integer(j.at("h"),8,3500));return r;}
struct Config{std::optional<Rect> tooltip,price,quantity,world;double interval=.8;std::string shop_name="Live-Shop",scan_mode="shop";std::int64_t default_quantity=1;bool overlay=false;};
Json config_json(const Config& c){return Json{{"tooltip",rect_json(c.tooltip)},{"price",rect_json(c.price)},{"quantity",rect_json(c.quantity)},{"worldRegion",world_rect_json(c.world)},{"interval",c.interval},{"shopName",c.shop_name},{"scanMode",c.scan_mode},{"defaultQuantity",c.default_quantity},{"overlay",c.overlay}};}
void update_config(Config& c,const Json& j){if(!j.is_object())throw std::runtime_error("Konfiguration muss ein Objekt sein");
  if(j.contains("tooltip"))c.tooltip=parse_rect(j.at("tooltip"));
  if(j.contains("price"))c.price=parse_rect(j.at("price"));
  if(j.contains("quantity"))c.quantity=parse_rect(j.at("quantity"));
  if(j.contains("worldRegion"))c.world=parse_world_rect(j.at("worldRegion"));
  if(j.contains("interval")){if(!j.at("interval").is_number())throw std::runtime_error("Intervall muss eine Zahl sein");c.interval=std::clamp(j.at("interval").get<double>(),.35,5.0);if(!std::isfinite(c.interval))throw std::runtime_error("Intervall ungueltig");}
  if(j.contains("shopName")){if(!j.at("shopName").is_string())throw std::runtime_error("Shopname ungueltig");c.shop_name=j.at("shopName").get<std::string>().substr(0,80);if(c.shop_name.empty())c.shop_name="Live-Shop";}
  if(j.contains("scanMode")){auto mode=j.at("scanMode").get<std::string>();if(mode!="shop"&&mode!="gather"&&mode!="world")throw std::runtime_error("Unbekannter Scanmodus");c.scan_mode=mode;}
  if(j.contains("defaultQuantity"))c.default_quantity=positive_integer(j.at("defaultQuantity"),1,100'000'000);
  if(j.contains("overlay")){if(!j.at("overlay").is_boolean())throw std::runtime_error("Overlay muss boolesch sein");c.overlay=j.at("overlay").get<bool>();}
}
std::vector<isya::Target> parse_targets(const Json& j){if(!j.is_array()||j.size()>600)throw std::runtime_error("Maximal 600 Suchziele erlaubt");std::vector<isya::Target> v;std::set<std::string> seen;
  for(const auto& x:j){if(!x.is_object()||!x.contains("name")||!x.at("name").is_string())throw std::runtime_error("Ungueltiger Itemname");
    isya::Target t;t.name=x.at("name").get<std::string>();if(t.name.size()<2||t.name.size()>120||isya::normalize(t.name).empty())throw std::runtime_error("Ungueltiger Itemname");
    if(!seen.insert(isya::normalize(t.name)).second)throw std::runtime_error("Doppelter Suchbegriff");
    t.need=positive_integer(x.at("need"),1,100'000'000);
    if(x.contains("maxUnit")&&!x.at("maxUnit").is_null())t.max_unit=positive_integer(x.at("maxUnit"),1,100'000'000'000'000LL);
    if(x.contains("source")&&x.at("source").is_string())t.source=x.at("source").get<std::string>().substr(0,150);
    v.push_back(std::move(t));
  }return v;
}
Json targets_json(const std::vector<isya::Target>& t){Json j=Json::array();for(const auto& x:t)j.push_back({{"name",x.name},{"need",x.need},{"maxUnit",x.max_unit?Json(*x.max_unit):Json(nullptr)},{"source",x.source}});return j;}

struct Frame {int w=0,h=0,ox=0,oy=0;std::vector<unsigned char> pixels;};
Frame grab(int x,int y,int w,int h){Frame f;f.w=w;f.h=h;f.ox=x;f.oy=y;if(w<1||h<1||w>12000||h>12000)throw std::runtime_error("Bildschirmaufnahme: ungueltige Groesse");
  HDC screen=GetDC(nullptr);if(!screen)throw std::runtime_error("Desktop-DC nicht verfuegbar");
  HDC mem=CreateCompatibleDC(screen);if(!mem){ReleaseDC(nullptr,screen);throw std::runtime_error("GDI-DC nicht verfuegbar");}
  BITMAPINFO bi{};bi.bmiHeader.biSize=sizeof(BITMAPINFOHEADER);bi.bmiHeader.biWidth=w;bi.bmiHeader.biHeight=-h;bi.bmiHeader.biPlanes=1;bi.bmiHeader.biBitCount=32;bi.bmiHeader.biCompression=BI_RGB;
  void* bits=nullptr;HBITMAP bmp=CreateDIBSection(screen,&bi,DIB_RGB_COLORS,&bits,nullptr,0);
  if(!bmp||!bits){DeleteDC(mem);ReleaseDC(nullptr,screen);throw std::runtime_error("GDI-Bitmap konnte nicht erzeugt werden");}
  HGDIOBJ old=SelectObject(mem,bmp);
  BOOL ok=BitBlt(mem,0,0,w,h,screen,x,y,SRCCOPY|CAPTUREBLT);
  if(ok)f.pixels.assign(static_cast<unsigned char*>(bits),static_cast<unsigned char*>(bits)+size_t(w)*size_t(h)*4);
  SelectObject(mem,old);DeleteObject(bmp);DeleteDC(mem);ReleaseDC(nullptr,screen);
  if(!ok)throw std::runtime_error("Bildschirmaufnahme fehlgeschlagen");return f;
}
Frame virtual_screen(){return grab(GetSystemMetrics(SM_XVIRTUALSCREEN),GetSystemMetrics(SM_YVIRTUALSCREEN),GetSystemMetrics(SM_CXVIRTUALSCREEN),GetSystemMetrics(SM_CYVIRTUALSCREEN));}
std::optional<Frame> grab_relative(const Rect& r,POINT cursor){
  int sx=GetSystemMetrics(SM_XVIRTUALSCREEN),sy=GetSystemMetrics(SM_YVIRTUALSCREEN);
  int ex=sx+GetSystemMetrics(SM_CXVIRTUALSCREEN),ey=sy+GetSystemMetrics(SM_CYVIRTUALSCREEN);
  int x=std::max(sx,static_cast<int>(cursor.x)+r.dx),y=std::max(sy,static_cast<int>(cursor.y)+r.dy);
  int right=std::min(ex,static_cast<int>(cursor.x)+r.dx+r.w),bottom=std::min(ey,static_cast<int>(cursor.y)+r.dy+r.h);
  if(right-x<8||bottom-y<8)return std::nullopt;
  return grab(x,y,right-x,bottom-y);
}
std::vector<unsigned char> bmp_bytes(const Frame& f){
  BITMAPFILEHEADER fh{};BITMAPINFOHEADER ih{};fh.bfType=0x4D42;fh.bfOffBits=sizeof(fh)+sizeof(ih);fh.bfSize=fh.bfOffBits+DWORD(f.pixels.size());
  ih.biSize=sizeof(ih);ih.biWidth=f.w;ih.biHeight=-f.h;ih.biPlanes=1;ih.biBitCount=32;ih.biCompression=BI_RGB;ih.biSizeImage=DWORD(f.pixels.size());
  std::vector<unsigned char> out(fh.bfSize);memcpy(out.data(),&fh,sizeof(fh));memcpy(out.data()+sizeof(fh),&ih,sizeof(ih));memcpy(out.data()+fh.bfOffBits,f.pixels.data(),f.pixels.size());return out;
}
Frame preview_frame(const Frame& in){double ratio=std::min(1.0,2000.0/double(std::max(in.w,in.h)));Frame out;out.ox=in.ox;out.oy=in.oy;out.w=std::max(1,int(std::round(in.w*ratio)));out.h=std::max(1,int(std::round(in.h*ratio)));out.pixels.resize(size_t(out.w)*out.h*4);
  for(int y=0;y<out.h;++y)for(int x=0;x<out.w;++x){int a=std::min(in.w-1,int(x/ratio)),b=std::min(in.h-1,int(y/ratio));memcpy(out.pixels.data()+4*(size_t(y)*out.w+x),in.pixels.data()+4*(size_t(b)*in.w+a),4);}
  return out;
}
Json snapshot_json(const Frame& frame,POINT cursor){Frame out=preview_frame(frame);return Json{{"base64","data:image/bmp;base64,"+base64(bmp_bytes(out))},{"width",frame.w},{"height",frame.h},{"origin",Json::array({frame.ox,frame.oy})},{"cursor",Json::array({cursor.x,cursor.y})},{"previewWidth",out.w},{"previewHeight",out.h}};}
class Ocr {
  tesseract::TessBaseAPI api_;bool ready_=false;std::string version_;
public:
  explicit Ocr(const fs::path& root){std::string dir="";const auto local=root/"tessdata";if(fs::exists(local))dir=local.string();const char* p=dir.empty()?nullptr:dir.c_str();
    if(api_.Init(p,"deu+eng",tesseract::OEM_LSTM_ONLY)!=0&&api_.Init(p,"eng",tesseract::OEM_LSTM_ONLY)!=0)return;
    ready_=true;version_=std::string("Tesseract ")+tesseract::TessBaseAPI::Version();api_.SetVariable("preserve_interword_spaces","1");
  }
  bool ready()const{return ready_;}
  std::string version()const{return ready_?version_:"nicht installiert / Sprachdaten fehlen";}
  std::string text(const std::optional<Frame>& input,bool single_line){if(!ready_||!input)return {};
    const Frame& f=*input;const int scale=2,w=std::min(3500,f.w*scale),h=std::min(3500,f.h*scale);
    std::vector<unsigned char> gray(size_t(w)*h);std::int64_t brightness=0;
    for(size_t i=0;i<f.pixels.size()/4;i++){int b=f.pixels[4*i],g=f.pixels[4*i+1],r=f.pixels[4*i+2];brightness+=(r*30+g*59+b*11)/100;}
    bool dark=brightness/(f.w*f.h)<120;
    for(int y=0;y<h;++y)for(int x=0;x<w;++x){int src=(y/scale)*f.w+(x/scale);int b=f.pixels[4*src],g=f.pixels[4*src+1],r=f.pixels[4*src+2];int v=(r*30+g*59+b*11)/100;if(dark)v=255-v;gray[size_t(y)*w+x]=static_cast<unsigned char>(std::clamp((v-128)*2+128,0,255));}
    api_.SetPageSegMode(single_line?tesseract::PSM_SINGLE_LINE:tesseract::PSM_SINGLE_BLOCK);api_.SetImage(gray.data(),w,h,1,w);
    if(api_.Recognize(nullptr)!=0)return {};
    std::unique_ptr<char[]> txt(api_.GetUTF8Text());std::string value=txt?std::string(txt.get()):"";api_.Clear();return value;
  }
  std::vector<isya::OcrLabel> labels(const Frame& f) {
    std::vector<isya::OcrLabel> out;
    if(!ready_||f.w<16||f.h<16)return out;
    // The world-mode OCR runs on visible pixels only. Scan a fixed calibrated area.
    std::vector<unsigned char> rgb(size_t(f.w)*f.h*3);
    for(size_t i=0;i<size_t(f.w)*f.h;i++){
      rgb[i*3]=f.pixels[i*4+2];rgb[i*3+1]=f.pixels[i*4+1];rgb[i*3+2]=f.pixels[i*4];
    }
    api_.SetPageSegMode(tesseract::PSM_SPARSE_TEXT);
    api_.SetImage(rgb.data(),f.w,f.h,3,f.w*3);
    if(api_.Recognize(nullptr)!=0){api_.Clear();return out;}
    auto* iter=api_.GetIterator();
    if(iter){
      do {
        std::unique_ptr<char[]> txt(iter->GetUTF8Text(tesseract::RIL_TEXTLINE));
        if(!txt)continue;
        int x0=0,y0=0,x1=0,y1=0;
        if(!iter->BoundingBox(tesseract::RIL_TEXTLINE,&x0,&y0,&x1,&y1))continue;
        float conf=iter->Confidence(tesseract::RIL_TEXTLINE);
        if(x1>x0&&y1>y0)out.push_back({txt.get(),x0,y0,x1-x0,y1-y0,conf});
      }while(out.size()<300 && iter->Next(tesseract::RIL_TEXTLINE));
    }
    api_.Clear();return out;
  }
};

Json match_json(const isya::Match& m){Json out={{"status",m.status}};if(m.status!="none"){
  out["name"]=m.name;out["need"]=m.need;out["maxUnit"]=m.max_unit?Json(*m.max_unit):Json(nullptr);out["unit"]=m.unit?Json(*m.unit):Json(nullptr);
  out["qtyAssumed"]=m.qty_assumed;out["message"]=m.message;
}return out;}
Json sample_json(const isya::Sample& s){return Json{{"name",s.name},{"price",s.price?Json(*s.price):Json(nullptr)},{"quantity",s.quantity},{"quantityAssumed",s.quantity_assumed},{"tooltipText",s.tooltip_text},{"priceText",s.price_text},{"quantityText",s.quantity_text},{"message",s.message}};}

class Engine {
  mutable std::mutex mutex_;Config config_;std::vector<isya::Target> targets_;isya::Sample preview_;
  Json pending_=Json::array(),capture_result_=nullptr;std::vector<isya::WorldHit> world_hits_;
  std::set<std::string> signatures_;std::string last_sig_,error_;int consecutive_=0;std::int64_t frames_=0;
  double due_=0;std::chrono::steady_clock::time_point last_scan_{};
  std::atomic<bool> active_{false};std::jthread worker_,snapshot_thread_;Ocr ocr_;fs::path root_;
  static double epoch(){return double(std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count())/1000.0;}
  void scan_loop(std::stop_token stop){while(!stop.stop_requested()){
    if(!active_){std::this_thread::sleep_for(100ms);continue;}
    Config c;{std::scoped_lock lk(mutex_);c=config_;}
    auto start=std::chrono::steady_clock::now();
    try{if(c.scan_mode=="world"){
        if(!c.world)throw std::runtime_error("Weltbereich kalibrieren");
        auto frame=grab(c.world->dx,c.world->dy,c.world->w,c.world->h);
        auto labels=ocr_.labels(frame);
        std::vector<isya::Target> targets;{std::scoped_lock lk(mutex_);targets=targets_;}
        auto hits=isya::find_visible_targets(labels,targets,frame.ox,frame.oy);
        {std::scoped_lock lk(mutex_);world_hits_=std::move(hits);last_scan_=std::chrono::steady_clock::now();++frames_;error_.clear();}
      }else{
      if(!c.tooltip||(c.scan_mode=="shop"&&!c.price))throw std::runtime_error("Tooltip-/Preisregion kalibrieren");
      POINT pos{};if(!GetCursorPos(&pos))throw std::runtime_error("Mausposition nicht verfuegbar");
      auto tip=ocr_.text(grab_relative(*c.tooltip,pos),false);
      auto price=c.scan_mode=="shop"?ocr_.text(grab_relative(*c.price,pos),true):"";
      auto qty=c.quantity?ocr_.text(grab_relative(*c.quantity,pos),true):"";
      isya::Sample sample;sample.tooltip_text=tip.substr(0,1200);sample.price_text=price.substr(0,600);sample.quantity_text=qty.substr(0,300);
      sample.name=isya::parse_name(tip);sample.price=isya::parse_price(price);auto detected=isya::parse_quantity(qty);sample.quantity_assumed=!detected;sample.quantity=detected.value_or(c.default_quantity);
      sample.message=c.scan_mode=="gather"?(sample.name.empty()?"Kein Rohstoffname erkannt":"Sammelobjekt erkannt"):(sample.name.empty()||!sample.price?"Tooltip oder Preis unklar: Region pruefen":"Artikel und Preis erfasst - noch nicht bestaetigt");
      std::scoped_lock lk(mutex_);preview_=sample;last_scan_=std::chrono::steady_clock::now();++frames_;error_.clear();
      if(c.scan_mode=="shop"&&!sample.name.empty()&&sample.price&&sample.quantity>0){
        auto sig=isya::signature(sample);consecutive_=(sig==last_sig_)?consecutive_+1:1;last_sig_=sig;
        if(consecutive_>=2&&signatures_.insert(sig).second){
          auto id=std::to_string(std::chrono::steady_clock::now().time_since_epoch().count())+std::to_string(frames_);
          pending_.push_back({{"id",id},{"name",sample.name},{"qty",sample.quantity},{"total",*sample.price},{"seller",c.shop_name},{"time",now_iso()},{"verified",false},{"quantityAssumed",sample.quantity_assumed},{"tooltipText",sample.tooltip_text},{"priceText",sample.price_text}});
          if(pending_.size()>250)pending_.erase(pending_.begin());
        }
      }else{consecutive_=0;last_sig_.clear();}
      } // non-world scanning
    }catch(const std::exception& ex){std::scoped_lock lk(mutex_);error_=ex.what();preview_.message=error_;}
    auto elapsed=std::chrono::steady_clock::now()-start;
    auto wait=std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::duration<double>(c.interval)-elapsed);
    if(wait>0ms)std::this_thread::sleep_for(wait);
  }}
public:
  explicit Engine(fs::path root):ocr_(root),root_(std::move(root)){
    try{auto text=read_file(root_/"live_scanner_config.json");if(!text.empty())update_config(config_,Json::parse(text));}catch(...){config_=Config{};}
    try{auto text=read_file(root_/"isya_watch_targets.json");if(!text.empty())targets_=parse_targets(Json::parse(text));}catch(...){targets_.clear();}
    worker_=std::jthread([this](std::stop_token stop){scan_loop(stop);});
  }
  ~Engine(){
    active_=false;
    worker_.request_stop();
    snapshot_thread_.request_stop();
    // Join while OCR/config and other engine members are still alive.
    if(worker_.joinable()) worker_.join();
    if(snapshot_thread_.joinable()) snapshot_thread_.join();
  }
  Json status()const{std::scoped_lock lk(mutex_);auto p=sample_json(preview_);Json hits=Json::array();for(const auto& hit:world_hits_)hits.push_back({{"name",hit.name},{"x",hit.x},{"y",hit.y},{"w",hit.width},{"h",hit.height},{"confidence",hit.confidence},{"category",hit.category}});bool fresh=last_scan_.time_since_epoch().count()&&std::chrono::steady_clock::now()-last_scan_<8s;
    p["match"]=(active_&&fresh)?match_json(isya::match_sample(preview_,targets_)):Json{{"status","none"}};
    return Json{{"worldHits",hits},{"version","4.1.0-cpp"},{"running",active_.load()},{"config",config_json(config_)},{"preview",p},{"pending",pending_},{"frames",frames_},{"error",error_},{"targetsCount",targets_.size()},{"captureDue",due_},{"ocrAvailable",ocr_.ready()?Json(ocr_.version()):Json(nullptr)}};
  }
  Json capture_status()const{std::scoped_lock lk(mutex_);return Json{{"image",capture_result_},{"due",due_}};}
  Json start(){if(!ocr_.ready())throw std::runtime_error("Native Tesseract-Sprachdateien fehlen. Siehe README / SETUP_TESSDATA.ps1.");{
      std::scoped_lock lk(mutex_);if(config_.scan_mode=="world"){if(!config_.world)throw std::runtime_error("Zuerst Weltbereich kalibrieren");}
      else if(!config_.tooltip||(config_.scan_mode=="shop"&&!config_.price))throw std::runtime_error("Zuerst Tooltip-/Preisbereich kalibrieren");
      world_hits_.clear();active_=true;}
    return status();}
  Json stop(){active_=false;{std::scoped_lock lk(mutex_);world_hits_.clear();}return status();}
  Json clear(){std::scoped_lock lk(mutex_);pending_=Json::array();signatures_.clear();last_sig_.clear();consecutive_=0;return Json{{"ok",true}};}
  Json update(const Json& input){std::scoped_lock lk(mutex_);Config copy=config_;update_config(copy,input);write_json(root_/"live_scanner_config.json",config_json(copy));config_=copy;return Json{{"ok",true}};}
  Json set_targets(const Json& j){auto next=parse_targets(j.at("targets"));std::scoped_lock lk(mutex_);write_json(root_/"isya_watch_targets.json",targets_json(next));targets_=std::move(next);return Json{{"ok",true}};}
  Json snapshot(int delay){if(delay<1||delay>12)throw std::runtime_error("Delay muss zwischen 1 und 12 Sekunden liegen");
    if(snapshot_thread_.joinable()){snapshot_thread_.request_stop();snapshot_thread_.join();}
    {std::scoped_lock lk(mutex_);due_=epoch()+delay;capture_result_=nullptr;}
    snapshot_thread_=std::jthread([this,delay](std::stop_token stop){for(int i=0;i<delay*10&&!stop.stop_requested();++i)std::this_thread::sleep_for(100ms);
      if(stop.stop_requested())return;
      Json result;try{POINT pos{};GetCursorPos(&pos);result=snapshot_json(virtual_screen(),pos);}catch(const std::exception& ex){result=Json{{"error",ex.what()}};}
      std::scoped_lock lk(mutex_);capture_result_=std::move(result);due_=0;
    });return Json{{"ok",true},{"due",epoch()+delay}};
  }
  struct OverlayStatus{bool visible=false;std::string status,name;std::int64_t need=0;std::optional<std::int64_t> unit;};
  OverlayStatus overlay_status()const{std::scoped_lock lk(mutex_);OverlayStatus out;if(!active_||!config_.overlay||!last_scan_.time_since_epoch().count()||std::chrono::steady_clock::now()-last_scan_>7s)return out;
    auto m=isya::match_sample(preview_,targets_);if(m.status=="none")return out;
    out.visible=true;out.status=m.status;out.name=m.name;out.need=m.need;out.unit=m.unit;return out;
  }
  std::vector<isya::WorldHit> world_hits()const{std::scoped_lock lk(mutex_);
    if(!active_||config_.scan_mode!="world"||!config_.overlay||std::chrono::steady_clock::now()-last_scan_>3s)return {};
    return world_hits_;
  }
  bool ocr_ready()const{return ocr_.ready();}
};

struct Overlay {Engine* engine;HWND hwnd=nullptr;Engine::OverlayStatus state;HFONT font=nullptr;};
LRESULT CALLBACK overlay_proc(HWND hwnd,UINT message,WPARAM wp,LPARAM lp){Overlay* ctx=reinterpret_cast<Overlay*>(GetWindowLongPtrW(hwnd,GWLP_USERDATA));
  if(message==WM_CREATE){auto* create=reinterpret_cast<CREATESTRUCTW*>(lp);SetWindowLongPtrW(hwnd,GWLP_USERDATA,reinterpret_cast<LONG_PTR>(create->lpCreateParams));return 0;}
  if(message==WM_NCHITTEST)return HTTRANSPARENT;
  if(message==WM_MOUSEACTIVATE)return MA_NOACTIVATE;
  if(message==WM_TIMER&&ctx){auto s=ctx->engine->overlay_status();bool changed=s.visible!=ctx->state.visible||s.status!=ctx->state.status||s.name!=ctx->state.name||s.unit!=ctx->state.unit;
    ctx->state=s;if(s.visible){if(changed){ShowWindow(hwnd,SW_SHOWNOACTIVATE);InvalidateRect(hwnd,nullptr,TRUE);}}else{ShowWindow(hwnd,SW_HIDE);}return 0;}
  if(message==WM_PAINT){PAINTSTRUCT ps;HDC dc=BeginPaint(hwnd,&ps);RECT r{};GetClientRect(hwnd,&r);HBRUSH brush=CreateSolidBrush(RGB(20,33,50));FillRect(dc,&r,brush);DeleteObject(brush);
    if(ctx&&ctx->state.visible){SetBkMode(dc,TRANSPARENT);SetTextColor(dc,ctx->state.status=="affordable"?RGB(160,237,189):ctx->state.status=="expensive"?RGB(255,170,164):ctx->state.status=="review"?RGB(244,200,114):RGB(145,213,255));
      if(ctx->font)SelectObject(dc,ctx->font);r.left+=12;r.top+=10;r.right-=10;
      std::string a=ctx->state.status=="affordable"?"IM PREISLIMIT":ctx->state.status=="expensive"?"ZU TEUER":ctx->state.status=="review"?"OCR-NAME PRUEFEN":"BENOETIGT";
      std::string line=a+"\n"+ctx->state.name+" | Noch "+std::to_string(ctx->state.need);
      if(ctx->state.unit)line+=" | "+std::to_string(*ctx->state.unit)+" Kupfer/Stk.";
      auto w=widen(line);DrawTextW(dc,w.c_str(),-1,&r,DT_LEFT|DT_WORDBREAK|DT_NOPREFIX);
    }EndPaint(hwnd,&ps);return 0;}
  if(message==WM_DESTROY){PostQuitMessage(0);return 0;}
  return DefWindowProcW(hwnd,message,wp,lp);
}
void overlay_loop(Engine& engine,std::stop_token stop){
  const wchar_t* cls=L"IsyaGoldNativeOverlay";WNDCLASSW wc{};wc.lpfnWndProc=overlay_proc;wc.hInstance=GetModuleHandleW(nullptr);wc.lpszClassName=cls;RegisterClassW(&wc);
  Overlay ctx{&engine};ctx.font=CreateFontW(19,0,0,0,FW_SEMIBOLD,FALSE,FALSE,FALSE,DEFAULT_CHARSET,OUT_DEFAULT_PRECIS,CLIP_DEFAULT_PRECIS,CLEARTYPE_QUALITY,DEFAULT_PITCH,L"Segoe UI");
  int x=GetSystemMetrics(SM_XVIRTUALSCREEN)+GetSystemMetrics(SM_CXVIRTUALSCREEN)-515;
  int y=GetSystemMetrics(SM_YVIRTUALSCREEN)+40;
  HWND hwnd=CreateWindowExW(WS_EX_LAYERED|WS_EX_TRANSPARENT|WS_EX_TOOLWINDOW|WS_EX_NOACTIVATE|WS_EX_TOPMOST,cls,L"Isya Gold Hinweis",WS_POPUP,x,y,495,112,nullptr,nullptr,GetModuleHandleW(nullptr),&ctx);
  if(!hwnd){DeleteObject(ctx.font);return;}ctx.hwnd=hwnd;SetLayeredWindowAttributes(hwnd,0,230,LWA_ALPHA);
  // Not guaranteed on every Windows build; best-effort exclusion from screenshots.
  SetWindowDisplayAffinity(hwnd,0x00000011);SetTimer(hwnd,1,400,nullptr);
  MSG msg;while(!stop.stop_requested()){
    while(PeekMessageW(&msg,nullptr,0,0,PM_REMOVE)){if(msg.message==WM_QUIT){DestroyWindow(hwnd);DeleteObject(ctx.font);return;}TranslateMessage(&msg);DispatchMessageW(&msg);}
    std::this_thread::sleep_for(35ms);
  }
  DestroyWindow(hwnd);DeleteObject(ctx.font);
}
struct WorldOverlayContext {Engine* engine=nullptr;std::vector<isya::WorldHit> hits;int origin_x=0,origin_y=0;HFONT font=nullptr;};
LRESULT CALLBACK world_overlay_proc(HWND hwnd,UINT msg,WPARAM wp,LPARAM lp){
  auto* ctx=reinterpret_cast<WorldOverlayContext*>(GetWindowLongPtrW(hwnd,GWLP_USERDATA));
  if(msg==WM_CREATE){auto* cs=reinterpret_cast<CREATESTRUCTW*>(lp);SetWindowLongPtrW(hwnd,GWLP_USERDATA,reinterpret_cast<LONG_PTR>(cs->lpCreateParams));return 0;}
  if(msg==WM_NCHITTEST)return HTTRANSPARENT;
  if(msg==WM_MOUSEACTIVATE)return MA_NOACTIVATE;
  if(msg==WM_TIMER && ctx){auto next=ctx->engine->world_hits();ctx->hits=std::move(next);InvalidateRect(hwnd,nullptr,FALSE);return 0;}
  if(msg==WM_ERASEBKGND)return 1;
  if(msg==WM_PAINT){PAINTSTRUCT ps;HDC dc=BeginPaint(hwnd,&ps);RECT rect{};GetClientRect(hwnd,&rect);auto bg=CreateSolidBrush(RGB(1,2,3));FillRect(dc,&rect,bg);DeleteObject(bg);
    if(ctx){SetBkMode(dc,TRANSPARENT);SetTextColor(dc,RGB(90,245,160));if(ctx->font)SelectObject(dc,ctx->font);
      auto pen=CreatePen(PS_SOLID,3,RGB(60,230,126));auto old=SelectObject(dc,pen);auto brush=SelectObject(dc,GetStockObject(HOLLOW_BRUSH));
      for(const auto& hit:ctx->hits){int left=hit.x-ctx->origin_x-5,top=hit.y-ctx->origin_y-5;
        Rectangle(dc,left,top,left+hit.width+10,top+hit.height+10);
        auto name=widen(hit.name);TextOutW(dc,left,top-23,name.c_str(),static_cast<int>(name.size()));
      }SelectObject(dc,brush);SelectObject(dc,old);DeleteObject(pen);
    }EndPaint(hwnd,&ps);return 0;
  }
  return DefWindowProcW(hwnd,msg,wp,lp);
}
void world_overlay_loop(Engine& engine,std::stop_token stop){
  const wchar_t* cls=L"IsyaGoldWorldMarkers";WNDCLASSW wc{};wc.lpfnWndProc=world_overlay_proc;wc.hInstance=GetModuleHandleW(nullptr);wc.lpszClassName=cls;RegisterClassW(&wc);
  WorldOverlayContext ctx;ctx.engine=&engine;ctx.origin_x=GetSystemMetrics(SM_XVIRTUALSCREEN);ctx.origin_y=GetSystemMetrics(SM_YVIRTUALSCREEN);
  ctx.font=CreateFontW(17,0,0,0,FW_BOLD,FALSE,FALSE,FALSE,DEFAULT_CHARSET,OUT_DEFAULT_PRECIS,CLIP_DEFAULT_PRECIS,CLEARTYPE_QUALITY,DEFAULT_PITCH,L"Segoe UI");
  HWND hwnd=CreateWindowExW(WS_EX_LAYERED|WS_EX_TRANSPARENT|WS_EX_TOOLWINDOW|WS_EX_NOACTIVATE|WS_EX_TOPMOST,
    cls,L"Isya Gold World Markers",WS_POPUP,ctx.origin_x,ctx.origin_y,
    GetSystemMetrics(SM_CXVIRTUALSCREEN),GetSystemMetrics(SM_CYVIRTUALSCREEN),nullptr,nullptr,GetModuleHandleW(nullptr),&ctx);
  if(!hwnd){DeleteObject(ctx.font);return;}
  SetLayeredWindowAttributes(hwnd,RGB(1,2,3),255,LWA_COLORKEY);
  SetWindowDisplayAffinity(hwnd,0x00000011);
  ShowWindow(hwnd,SW_SHOWNOACTIVATE);SetTimer(hwnd,1,500,nullptr);
  MSG msg;while(!stop.stop_requested()){
    while(PeekMessageW(&msg,hwnd,0,0,PM_REMOVE)){TranslateMessage(&msg);DispatchMessageW(&msg);}
    std::this_thread::sleep_for(40ms);
  }
  DestroyWindow(hwnd);DeleteObject(ctx.font);
}
void json_response(httplib::Response& res,const Json& j,int code=200){res.status=code;res.set_content(j.dump(-1,' ',false,Json::error_handler_t::replace),"application/json; charset=utf-8");res.set_header("Cache-Control","no-store");res.set_header("X-Content-Type-Options","nosniff");}
bool allowed(const httplib::Request& req){auto host=req.get_header_value("Host");auto origin=req.get_header_value("Origin");return (host.empty()||host=="127.0.0.1:8765") && (origin.empty()||origin=="http://127.0.0.1:8765");}
std::string mime(const std::string& path){if(path.ends_with(".html"))return "text/html; charset=utf-8";if(path.ends_with(".js"))return "text/javascript; charset=utf-8";if(path.ends_with(".css"))return "text/css; charset=utf-8";return "application/octet-stream";}
}
int main(){
  SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);
  try{
    const auto root=app_dir();Engine engine(root);
    std::jthread overlay([&engine](std::stop_token stop){overlay_loop(engine,stop);});
    std::jthread world_overlay([&engine](std::stop_token stop){world_overlay_loop(engine,stop);});
    httplib::Server server;
    server.set_payload_max_length(12000);
    server.set_read_timeout(5,0);
    server.Get("/api/status",[&](const httplib::Request& req,httplib::Response& res){if(!allowed(req)){json_response(res,{{"error","Nur localhost"}},403);return;}json_response(res,engine.status());});
    server.Get("/api/capture",[&](const httplib::Request& req,httplib::Response& res){if(!allowed(req)){json_response(res,{{"error","Nur localhost"}},403);return;}json_response(res,engine.capture_status());});
    server.Post(R"(/api/(start|stop|config|targets|clear|snapshot))",[&](const httplib::Request& req,httplib::Response& res){
      if(!allowed(req)){json_response(res,{{"error","Nur localhost"}},403);return;}
      try{auto obj=Json::parse(req.body.empty()?"{}":req.body);if(!obj.is_object())throw std::runtime_error("JSON-Objekt erwartet");
        auto path=req.path;Json data;
        if(path=="/api/start")data=engine.start();
        else if(path=="/api/stop")data=engine.stop();
        else if(path=="/api/config"){engine.update(obj);data=engine.status();}
        else if(path=="/api/targets"){engine.set_targets(obj);data=engine.status();}
        else if(path=="/api/clear")data=engine.clear();
        else if(path=="/api/snapshot")data=engine.snapshot(int(positive_integer(obj.value("delay",Json(5)),1,12)));
        else throw std::runtime_error("Unbekannte API");
        json_response(res,data);
      }catch(const std::exception& ex){json_response(res,{{"error",ex.what()}},400);}
    });
    std::set<std::string> assets={"index.html","catalog.js","expansion.js","upgrade.css","watchlist.js","watchlist.css","live_scan.js","live_scan.css"};
    server.Get(R"(/.*)",[&](const httplib::Request& req,httplib::Response& res){
      if(!allowed(req)){res.status=403;return;}
      std::string name=req.path=="/"?"index.html":req.path.substr(1);
      if(!assets.contains(name)){res.status=404;return;}
      auto bytes=read_file(root/name);if(bytes.empty()){res.status=404;return;}
      res.set_content(bytes,mime(name));res.set_header("Cache-Control","no-store");
      res.set_header("X-Content-Type-Options","nosniff");
    });
    std::cout<<"Isya Gold Native C++ 4.1 | OCR: "<<(engine.ocr_ready()?"bereit":"Sprachdaten fehlen")<<"\n";
    std::cout<<"Nur Bildschirmaufnahme. Keine Spielsteuerung. http://127.0.0.1:8765/\n";
    std::jthread browser([](std::stop_token){std::this_thread::sleep_for(900ms);ShellExecuteW(nullptr,L"open",L"http://127.0.0.1:8765/",nullptr,nullptr,SW_SHOWNORMAL);});
    if(!server.listen("127.0.0.1",kPort)){std::cerr<<"Port 8765 ist bereits belegt. Python Scanner vorher beenden.\n";return 2;}
    overlay.request_stop();world_overlay.request_stop();return 0;
  }catch(const std::exception& ex){std::cerr<<"Isya Gold Fehler: "<<ex.what()<<"\n";return 1;}
}
#endif
