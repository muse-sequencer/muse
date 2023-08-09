include(CheckCXXSourceRuns)

function(check_istringstream_hexfloat varname)
  check_cxx_source_runs("
  #include <sstream>
  #include <clocale>
  int main() {
  std::istringstream ss(\"0x1.8p+0\");
  ss.imbue(std::locale("C"));
  double value = 0.0;
  ss >> value;
  return value != 1.5;
  }
  " ${varname})
endfunction()
