#include <TableWriter.hpp>

int main(int argc, char *argv[])
{
  Table t({
      {20, "headerEins"},
      {12, "header2"},
      {15, "header3"}
    });
  t.printHeader();

  t.printLine("abc123", "def", 456);
  t.printLine("abc23", "DEF", 4567);
}
