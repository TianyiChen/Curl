#include<curl/curl.h>
#include<cstring>
#include<cassert>
#include<iostream>
#include<string>
#include<mutex>
#include<string_view>
namespace tianyichen{
enum METHOD{
	METHOD_GET,
	METHOD_POST,
	METHOD_PUT,
	METHOD_DELETE
};
std::string_view METHOD_STR[]={"GET","POST","PUT","DELETE"};
struct _CurlInit{
	_CurlInit(){curl_global_init(CURL_GLOBAL_DEFAULT);}
	~_CurlInit(){curl_global_cleanup();}
}_CurlInit;
using std::string;
struct MyHTTPException:std::exception{
	int code;
	string s;
	MyHTTPException()=delete;
	MyHTTPException(int code,string&S):code{code},s(move(S)){}
	MyHTTPException(int code,string&&S):code{code},s(move(S)){}
	virtual const char*what()const noexcept{ return s.data(); }
	void dump(){ std::cerr<<code<<' '<<s<<std::endl; }
};
template<size_t buffer_size>
struct CurlDedicatedStorage{
	char http_buffer[buffer_size+16],*http_buffer_end;
};
template<size_t buffer_size>
struct CurlSharedStorage{
	static std::mutex locker;
	static char http_buffer[buffer_size+16],*http_buffer_end;
	void lock(){ locker.lock(); }
	void unlock(){ locker.unlock(); }
};
template<size_t s> std::mutex CurlSharedStorage<s>::locker;
template<size_t s> char CurlSharedStorage<s>::http_buffer[s+16];
template<size_t s> char*CurlSharedStorage<s>::http_buffer_end;

template<size_t buffer_size=1<<20,bool throw_exception=1,template<size_t>class Storage=CurlDedicatedStorage>
struct Curl:Storage<buffer_size>{
	CURL*h;
	curl_slist*header_list=0;
	long status_code;
	CURLcode r;//r:return value of perform
	static size_t _write_callback(char *ptr,size_t _size_,size_t nmemb,
		Curl<buffer_size,throw_exception,Storage> *c){
		if(c->http_buffer_end+nmemb>c->http_buffer+buffer_size)return 0;
		memcpy(c->http_buffer_end,ptr,nmemb);
		c->http_buffer_end+=nmemb;
		return nmemb;
	}
	Curl(){
		h=curl_easy_init();
		curl_easy_setopt(h,CURLOPT_WRITEFUNCTION,_write_callback);
		curl_easy_setopt(h,CURLOPT_WRITEDATA,this);
		curl_easy_setopt(h,CURLOPT_TCP_KEEPALIVE,1);
	}
	~Curl(){
		curl_easy_cleanup(h);
		curl_slist_free_all(header_list);
	}
	void set_url(const char*s){ curl_easy_setopt(h,CURLOPT_URL,s); }
	void set_timeout(long timeout_in_ms){curl_easy_setopt(h,CURLOPT_TIMEOUT_MS,timeout_in_ms);}
	void header_append(const char*s){ header_list=curl_slist_append(header_list,s); }
	void header_reset(){ curl_slist_free_all(header_list); header_list=0; }
	void set_method(METHOD m){
		switch(m){
		case METHOD_POST:curl_easy_setopt(h,CURLOPT_CUSTOMREQUEST,"POST");break;
		case METHOD_PUT:curl_easy_setopt(h,CURLOPT_CUSTOMREQUEST,"PUT");break;
		case METHOD_DELETE:curl_easy_setopt(h,CURLOPT_CUSTOMREQUEST,"DELETE");break;
		case METHOD_GET:curl_easy_setopt(h,CURLOPT_CUSTOMREQUEST,"GET");break;
		default:assert(!"Invalid method");
		}
	}
	void set_post_json(){header_append("Content-type: application/json");}
	void set_post_data(std::string_view data){
		curl_easy_setopt(h,CURLOPT_POSTFIELDS,data.data());
		header_append(("Content-Length: "+std::to_string(data.size())).data());
	}
	long perform(){
		this->http_buffer_end=this->http_buffer;
		curl_easy_setopt(h,CURLOPT_HTTPHEADER,header_list);
		r=curl_easy_perform(h);
		*this->http_buffer_end=0;
		curl_easy_getinfo(h,CURLINFO_RESPONSE_CODE,&status_code);
		if constexpr(throw_exception){
			if(r)throw MyHTTPException(0,curl_easy_strerror(r));
			if(status_code!=200)throw MyHTTPException(status_code,this->http_buffer);
		}
		return r;
	}
};
}
