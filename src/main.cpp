#include "FTXAPI.h"

using namespace std;

int main(int argc, char **argv)
{
	FTXAPI ftxapi;
	//	ftxapi.run();
	string eth = "ETHUSDT";
	double number = 0.004;
	ftxapi.buy(eth, 2800, number);
	ftxapi.sell(eth, 3500, number);
	ftxapi.getBuyOrderNumberAndMax(eth);
	ftxapi.getPrices(eth);
	return 0;
}
