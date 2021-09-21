#ifndef FTX_API_H
#define FTX_API_H

#include "ls/exchange/API.h"

class FTXAPI : public ls::exchange::API
{
	public:
		std::vector<double> getPrices(const std::string &coin) override;
		std::pair<int, double> getBuyOrderNumberAndMax(const std::string &coin) override;
	protected:
		std::string order(const std::string &coin, double price, double number, const std::string &side) override;
		std::string getTimeStamp() override;
};

#endif
