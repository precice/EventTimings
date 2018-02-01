#include "TableWriter.hpp"

#include <algorithm>
#include <iostream>
#include <iomanip>
#include <numeric>
#include <string>
#include <vector>


Column::Column(std::string const & colName) : name(colName)
{
  width = name.size();
}

Column::Column(std::string const & colName, int colWidth) :
  name(colName),
  width(colWidth)
{}
