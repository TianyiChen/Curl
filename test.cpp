#include "cppcurl.h"
#include<iostream>
using namespace std;
using namespace tianyichen;
Curl<> c;
Curl<1<<20,0,CurlSharedStorage> cs;
int main(){
	try{
		c.set_url("https://httpbin.org/get");
		c.set_timeout(5000);
		cerr<<c.perform()<<endl;
		cerr<<c.status_code<<endl;
		cerr<<c.http_buffer<<endl;
	} catch(exception&ex){
		cerr<<ex.what()<<endl;
	}
	cs.lock();
	cs.set_url("https://httpbin.org/status/500");
	cs.set_method(METHOD_POST);
	cs.set_post_json();
	cs.set_post_data(R"({"key":123})");
	cerr<<cs.perform()<<' '<<cs.status_code<<endl;

	cs.set_url("https://httpbin.org/put");
	cs.set_method(METHOD_PUT);
	cerr<<cs.perform()<<' '<<cs.status_code<<endl;
	cerr<<cs.http_buffer<<endl;
	cs.unlock();
}
