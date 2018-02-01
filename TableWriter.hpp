#include <algorithm>
#include <chrono>
#include <iostream>
#include <iomanip>
#include <numeric>
#include <string>
#include <vector>

struct Column
{
  std::string name;
  int width;

  explicit Column(std::string const & colName);
  
  Column(std::string const & colName, int colWidth);
};


class Table
{
public:

  std::vector<Column> cols;
  std::string sepChar = "|";
  std::string padding = " ";
  std::ostream* out = &std::cout;

  Table() {};
  
  explicit Table(std::initializer_list<std::string> headers)
  {
    for (auto & h : headers) {
      cols.emplace_back(h);
    }    
  }

  explicit Table(std::initializer_list<std::pair<int, std::string>> headers)
  {
    for (auto & h : headers) {
      cols.emplace_back(std::get<1>(h), std::get<0>(h));
    }    
  }

  void printHeader()
  {
    using namespace std;

    for (auto & h : cols) {
      *out << padding << setw(h.width) << h.name << padding << sepChar;
    }
    
    *out << endl;
    int headerLength = std::accumulate(cols.begin(), cols.end(), 0, [](int count, Column col){
        return count + col.width + 3;
      });
    std::string sepLine(headerLength, '-');
    *out << sepLine << endl;
  }

  template<class T>
  void printLine(size_t index, T a)
  {
    int width = index < cols.size() ? cols[index].width : 0;
    *out << padding << std::setw(width) << a << padding << sepChar << std::endl;
  }

  template<class T, class ... Ts>
  void printLine(size_t index, T a, Ts... args)
  {
    int width = index < cols.size() ? cols[index].width : 0;
    *out << padding << std::setw(width) << a << padding << sepChar;
    printLine(index+1, args...);    
  }

  template<class Rep, class Period, class ... Ts>
  void printLine(size_t index, std::chrono::duration<Rep, Period> duration, Ts... args)
  {
    int width = index < cols.size() ? cols[index].width : 0;
    double ms = std::chrono::duration_cast<std::chrono::milliseconds>(duration).count();
    *out << padding << std::setw(width) << ms << padding << sepChar;
    printLine(index+1, args...);    
  }

  template<class ... Ts>
  void printLine(Ts... args)
  {
    printLine(static_cast<size_t>(0), args...);    
  }
    
};
