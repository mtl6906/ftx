#include "FTXAPI.h"
#include "ls/cstring/API.h"

using namespace std;
using namespace ls;

vector<double> FTXAPI::getPrices(const string &coin)
{
	vector<double> prices(2);
	string text = transacation("GET", string("/api/markets/") + coin + "/orderbook?depth=1");
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

string FTXAPI::order(const string &coin, double price, double number, const string &type)
{
	map<string, string> attribute;
	attribute["Content-Type"] = "application/json";
	attribute["FTX-KEY"] = config.apiKey;
	json::Object root;
	json::api.push(root, "market", coin.c_str());
	json::api.push(root, "side", "sell");
	json::api.push(root, "price", price);
	json::api.push(root, "type", "limit");
	json::api.push(root, "size", number);
	json::api.push(root, "postOnly", true);
	string body = root.toString();
	attribute["FTX-SIGN"] = signature({attribute["FTX-TS"] = getTimeStamp(), "POST", "/api/orders", body});
	return transacation("POST", "/api/orders", body, attribute);
}

pair<int, double> FTXAPI::getBuyOrderNumberAndMax(const string &coin)
{
	int count = 0;
	double maxPrice = 0;
	
	map<string, string> attribute;
	attribute["FTX-KEY"] = config.apiKey;
	string url = "/api/orders?market=" + coin;
	attribute["FTX-SIGN"] = signature({attribute["FTX-TS"] = getTimeStamp(), "GET", url, ""});
	auto responseText = transacation("GET", url, "", attribute);

	json::Object root = json::api.decode(responseText);
	json::Array result;
	json::api.get(root, "result", result);
	for(int i=0;i<result.size();++i)
	{
		json::Object it;
		string type;
		json::api.get(result, i, it);
		json::api.get(it, "side", type);
		if(type == config.buyText)
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

std::string FTXAPI::getTimeStamp()
{
	return to_string(1000 * (time(NULL) - 8 * 3600));
}
