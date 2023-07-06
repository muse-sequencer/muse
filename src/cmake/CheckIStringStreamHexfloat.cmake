include(CheckCXXSourceRuns)

function(check_istringstream_hexfloat varname)
  check_cxx_source_runs("
  #include <sstream>
  int main() {
  double value = 0;
  std::istringstream(\"0x1.8p+0\") >> value;
  return value != 1.5;
  }
  " ${varname})
endfunction()
