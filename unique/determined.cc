#include <signal.h>
#include <iostream>
#include <fstream>
#include <tuple>
#include <map>
#include <memory>
#include <string>

#include <docopt.h>
#include <docopt_util.h>

#include "extractor.h"
#include "QBFParser.h"
#include "QCIRParser.h"
#include "DQCIRParser.h"
#include "QDIMACSParser.h"
#include "DQDIMACSParser.h"

using std::string;

static const char USAGE[] =
R"(Usage: 
  unique [options] <input file>

Options:
  -h --help                     Show this screen.
  -c --conflict-limit <int>     Conflict limit for SAT solver (per variable). [default: 1000]
  -o --output-file <filename>   Write output to file (instead of standard output).
  -s --strict                   Use only universals for existential definitions and vice versa.
  --output-format <format>      Output format [default: QCIR]
                                (QCIR | QDIMACS | DIMACS | Verilog)
  --ordering-file <filename>    Read variable ordering for definability from file.               
)";

static volatile sig_atomic_t sig_caught = 0;
static unique_ptr<Extractor> extractor = nullptr;

void handle_sighup(int signum) 
{
  std::cerr << "Received signal " << signum << ", trying to shut down gracefully." << std::endl;
  sig_caught = 1;
  if (extractor) {
    extractor->interrupt();
  }
}

enum class filetype: int { None, QDIMACS, DQDIMACS, QCIR, DQCIR };

filetype checkFileType(const string& filename) {
  auto file = std::ifstream(filename);
  if (file.good()) {
    string first_line;
    std::getline(file, first_line);
    string qcir_start = "#QCIR";
    bool qcir = (first_line.substr(0, qcir_start.length()) == qcir_start);
    // Now determine if there are explicit dependencies.
    bool explicit_dependencies = false;
    string line;
    while (std::getline(file, line)) {
      if (line.length() > 0 && line.front() == 'd') {
        explicit_dependencies = true;
        break;
      }
    }
    if (qcir && !explicit_dependencies) {
      return filetype::QCIR;
    } else if (qcir && explicit_dependencies) {
      return filetype::DQCIR;
    } else if (!qcir && !explicit_dependencies) {
      return filetype::QDIMACS;
    } else if (!qcir && explicit_dependencies) {
      return filetype::DQDIMACS;
    } else {
      return filetype::None;
    }
  }
  return filetype::None;
}

int main(int argc, char* argv[]) {
  std::map<std::string, docopt::value> args = docopt::docopt(USAGE, { argv + 1, argv + argc }, true, "Unique v.0.1");

  unique_ptr<QBFParser> parser;
  string input_filename = args["<input file>"].asString();

  filetype input_filetype = checkFileType(input_filename);

  switch(input_filetype) {
    case filetype::QDIMACS:
      parser = std::make_unique<QDIMACSParser>(input_filename);
      std::cerr << "Reading QDIMACS file: " << input_filename << std::endl;
      break;
    case filetype::QCIR:
      parser = std::make_unique<QCIRParser>(input_filename);
      std::cerr << "Reading QCIR file: " << input_filename << std::endl;
      break;
    case filetype::DQCIR:
      parser = std::make_unique<DQCIRParser>(input_filename);
      break;
    case filetype::DQDIMACS:
      parser = std::make_unique<DQDIMACSParser>(input_filename);
      std::cerr << "Reading DQDIMACS file: " << input_filename << std::endl;
      break;
    case filetype::None:
      std::cerr << "Invalid input file: " << input_filename << std::endl;
      return 1;
  }

  if (args["--ordering-file"]) {
    std::cerr << "Using ordering file: " << args["--ordering-file"].asString() << std::endl;
    parser->setComparator(args["--ordering-file"].asString());
  }

  extractor = std::make_unique<Extractor>(args["--conflict-limit"].asLong(), !args["--strict"].asBool());

  signal(SIGINT,  handle_sighup);
  signal(SIGTERM, handle_sighup);
  signal(SIGXCPU, handle_sighup);

  try {
    parser->getDefinitions(*extractor);
  }
  catch (std::bad_alloc&) {
    std::cerr << "Out of memory." << std::endl;
    extractor.reset();
  }

  if (args["--output-file"]) {
    string output_filename = args["--output-file"].asString();
    std::cerr << "Writing to file: " << output_filename << std::endl;
    if (args["--output-format"].asString() == "QDIMACS") {
      parser->writeQDIMACS(output_filename);
    } else if (args["--output-format"].asString() == "DIMACS") {
      parser->writeDIMACS(output_filename);
    } else if (args["--output-format"].asString() == "QCIR") {
      parser->writeQCIR(output_filename);
    } else if (args["--output-format"].asString() == "Verilog") {
      parser->writeVerilog(output_filename);
    } else {
      std::cerr << "Invalid output format: " << args["--output-format"].asString() << ", using default (QCIR)." << std::endl;
      parser->writeQCIR(output_filename);
    }
  } else {
    if (args["--output-format"].asString() == "QDIMACS") {
      parser->writeQDIMACS();
    } else if (args["--output-format"].asString() == "DIMACS") {
      parser->writeDIMACS();
    } else if (args["--output-format"].asString() == "QCIR") {
      parser->writeQCIR();
    } else if (args["--output-format"].asString() == "Verilog") {
      parser->writeVerilog();
    } else {
      std::cerr << "Invalid output format: " << args["--output-format"].asString() << ", using default (QCIR)." << std::endl;
      parser->writeQCIR();
    }
  }
  
  return 0;
}

