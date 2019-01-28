#include <TableWriter.hpp>

using namespace std;

int main(int argc, char *argv[])
{
  Table t;
  t.addColumn("headerEins");
  t.addColumn("header2");
  t.addColumn("n", 10, 2);
  t.addColumn("foo", 10, 2);
  t.printHeader();

  t.printRow("abc123", "def", 46, 1);
  t.printRow("abc23", "DEF", 10.0/3, 10.0/3);

  cout << endl;
  cout << "|" << setw(20) << setprecision(3) << 10.0/3 << "|" << endl;
  
}
