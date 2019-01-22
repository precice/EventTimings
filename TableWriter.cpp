#include "TableWriter.hpp"

#include <algorithm>
#include <iostream>
#include <iomanip>
#include <numeric>
#include <string>
#include <vector>


Column::Column(std::string const & name)
  : name(name),
    width(name.size())
{}

Column::Column(std::string const & name, int width) :
  name(name),
  width(std::max(width, static_cast<int>(name.size())))
{}

Table::Table(std::initializer_list<std::string> headers)
{
  for (auto & h : headers) {
    cols.emplace_back(h);
  }    
}

Table::Table(std::initializer_list<std::pair<int, std::string>> headers)
{
  for (auto & h : headers) {
    cols.emplace_back(std::get<1>(h), std::get<0>(h));
  }    
}

void Table::printHeader()
{
  using namespace std;

  for (auto & h : cols) {
    *out << padding << setw(h.width) << h.name << padding << sepChar;
  }
    
  *out << endl;
  int headerLength = std::accumulate(cols.begin(), cols.end(), 0, [this](int count, Column col){
    return count + col.width + sepChar.size() + 2;
    });
  std::string sepLine(headerLength, '-');
  *out << sepLine << endl;
}
