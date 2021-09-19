#include "ls/http/Request.h"
#include "ls/http/QueryString.h"
#include "ls/io/InputStream.h"
#include "ls/io/OutputStream.h"
#include "ls/ssl/Client.h"
#include "ls/net/Client.h"
#include "ls/json/API.h"
#include "ls/SHA256.h"
#include "ls/DefaultLogger.h"
#include "string"
#include "vector"
#include "iostream"
#include "memory"
#include "stack"
#include "unistd.h"

using namespace ls;
using namespace std;

char *ip, *apiURL, *secretKey, *apiKey;
double rate, uprate;
char *coin;

string transacation(const string &method, const string &url, const string &body = "", const map<string, string> &attributes = map<string, string>())
{
	http::Request request;
	request.setDefaultHeader();
	request.getMethod() = method;
	request.getURL() = url;
	request.getBody() = body;
	request.getVersion() = "HTTP/1.1";
	request.setAttribute("Host", apiURL);
	request.setAttribute("User-Agent", "Mozilla/5.0 (X11; Ubuntu; Linux x86_64; rv:89.0) Gecko/20100101 Firefox/89.0");
	if(body != "")
		request.setAttribute("Content-Length", to_string(body.size()));
	for(auto &attribute : attributes)
		request.setAttribute(attribute.first, attribute.second);
	ssl::Client sslClient;
	unique_ptr<ssl::Connection> connection(sslClient.getConnection(net::Client(ip, 443).connect()));
	connection -> setHostname(url);
	connection -> connect();
	
	io::OutputStream out(connection -> getWriter(), new Buffer());
	string text = request.toString();
	
	LOGGER(ls::INFO) << "request:\n" << text << ls::endl;
	
	out.append(text);
	out.write();

	LOGGER(ls::INFO) << "cmd sending..." << ls::endl;

	io::InputStream in(connection -> getReader(), new Buffer());
	in.read();

	LOGGER(ls::INFO) << "reading..." << ls::endl;

	in.split("\r\n\r\n", true);
	return in.split();
}

string signature(const string &ts, const string &method, const string &path, const string &body)
{
	string signaturePayload = ts + method + path + body;
	ls::SHA256 sha256;
	return sha256.hmac(signaturePayload, secretKey);
}

vector<double> getPrice(const string &coin)
{
	vector<double> prices(2);
	string text = transacation("GET", string("/markets/") + coin + "/orderbook?depth=1");
	cout << text << endl;
	auto root = json::api.decode(text);
	json::Object result;
	json::Array bids;
	json::Array bid;
	json::api.get(result, "bids", bids);
	json::api.get(bids, 0, bid);
	json::api.get(bid, 0, prices[0]);
	json::Array asks;
	json::Array ask;
	json::api.get(root, "result", result);
	json::api.get(result, "asks", asks);
	json::api.get(asks, 0, ask);
	json::api.get(ask, 0, prices[1]);
	return prices;
}


string order(const string &coin, double price, double number, const string &type)
{
	map<string, string> attribute;
	attribute["Content-Type"] = "application/json";
	attribute["FTX-KEY"] = apiKey;
	json::Object root;
	json::api.push(root, "market", coin.c_str());
	json::api.push(root, "side", "sell");
	json::api.push(root, "price", price);
	json::api.push(root, "type", "limit");
	json::api.push(root, "size", number);
	json::api.push(root, "postOnly", true);
	string body = root.toString();
	attribute["FTX-SIGN"] = signature(attribute["FTX-TS"] = to_string(time(NULL) * 1000), "POST", "/orders", body);
	return transacation("POST", "/orders", body, attribute);
}

void buy(const string &coin, double price, double number)
{
	auto text = order(coin, price, number, "buy");
	LOGGER(ls::INFO) << text << ls::endl;
}

void sell(const string &coin, double price, double number)
{
	auto text = order(coin, price, number, "sell");
	LOGGER(ls::INFO) << text << ls::endl;
}

pair<int, double> getBuyOrderNumberAndMax(const string &coin)
{
	int count = 0;
	double maxPrice = 0;
	
	map<string, string> attribute;
	attribute["FTX-KEY"] = apiKey;
	string url = "/orders?market=" + coin;
	attribute["FTX-SIGN"] = signature(attribute["FTX-TS"] = to_string(1000 * time(NULL)), "GET", url, "");
	auto responseText = transacation("GET", url, "", attribute);

	LOGGER(ls::INFO) << responseText << ls::endl;

	json::Object root = json::api.decode(responseText);
	json::Array result;
	json::api.get(root, "result", result);
	for(int i=0;i<result.size();++i)
	{
		json::Object it;
		string type;
		json::api.get(result, i, it);
		json::api.get(it, "side", type);
		if(type == "BUY")
		{
			double price;
			++count;
			json::api.get(it, "price", price);
			int p1 = (int)(maxPrice * 100), p2 = (int)(price * 100);
			if(p1 < p2)
				maxPrice = price;
		}
	}
	pair<int, double> r;
	r.first = count;
	r.second = maxPrice;
	return r;
}

double round2(double value)
{
	int v = value * 100;
	return v / 100.0;
}

void method()
{
	for(;;)
	{
		sleep(2);
		auto prices = getPrice(coin);
		auto buyOrderNumber = getBuyOrderNumberAndMax(coin);
		if(buyOrderNumber.first == 0)
		{
			sell(coin, prices[0], 0.2);
			buy(coin, round2(prices[0] * (1 - rate)), 0.2);
		}
		else
		{
			if(buyOrderNumber.first >= 5)
				continue;
			long long currentPrice = (long long)(prices[0] * 10000);
			long long signPriceNow = (long long)(buyOrderNumber.second * (1 + uprate) * 10000);
			if(currentPrice > signPriceNow)
			{
				sell(coin, prices[0], 0.2);
				buy(coin, round2(prices[0] * (1 - rate)), 0.2);
			}
		}
	}
}

int main(int argc, char **argv)
{
	ip = argv[1];      
	apiURL = argv[2];
	apiKey = argv[3];
	secretKey = argv[4];
	rate = stod(argv[5]);
	uprate = stod(argv[6]);
	coin = argv[7];
	LOGGER(ls::INFO) << "rate: " << rate << ls::endl;
	getPrice(coin);
	buy(coin, 150, 0.01);
//	cout << sell("ARUSDT", 90, 0.3) << endl;
	getBuyOrderNumberAndMax(coin);
//	method();
}
