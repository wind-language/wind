#include <wind/reporter/parser.h>
#include <string>
#include <vector>
#include <iostream>
#include <sstream>
#include <iterator>
#include <wind/isc/isc.h>

std::string ParserReport::line(uint16_t t_line) {
    std::string line;
    int i = 0;
    while (std::getline(this->src, line)) {
        i++;
        if (i == t_line) {
            ssize_t pos = 0;
            while ((pos = line.find('\t', pos)) != (ssize_t)std::string::npos) {
                line.replace(pos, 1, " ");
                pos += 2;
            }
            return line;
        }
    }
    return "";
}

#define LNUM_ADAPT_LEN 5
void ReportLine(std::string line, TokenPos start, TokenPos end) {
    std::string line_num = std::to_string(start.first);
    line_num = std::string(LNUM_ADAPT_LEN - line_num.size(), ' ') + line_num;
    line_num = line_num + " | ";
    std::cerr << line_num << line << "\x1b[0m" << std::endl;
    std::cerr << std::string(line_num.size()+start.second-1, ' ');
    std::cerr << "\x1b[32m" << std::string(end.second-start.second, '^') << "\x1b[0m" << std::endl;
}

void ParserReport::Report(ParserReport::Type type, Token *expecting, Token *found) {
    std::string path = global_isc->getPath(found->srcId);
    if (type == ParserReport::Type::PARSER_WARNING) {
        std::cerr << "\x1b[33m[WARNING] \x1b[0m\x1b[1m" << "Unexpected token" << "\x1b[0m" << std::endl;
    } else if (type == ParserReport::Type::PARSER_ERROR) {
        std::cerr << "\x1b[31m[ERROR] \x1b[0m\x1b[1m" << "Unexpected token" << "\x1b[0m" << std::endl;
    }

    if (!expecting || !found) {
        return;
    }

    std::cerr << "\x1b[1m" << "Expecting: " << "\x1b[0m" << expecting->name;
    if (expecting->type == Token::Type::SEMICOLON) {
        std::cerr << " (Expression parsing)";
    }
    std::cerr << std::endl;
    std::cerr << "\x1b[1m" << "Found: `" << "\x1b[0m" << found->name << "\x1b[0m`\x1b[36m (" 
              << found->range.first.first << ":" << found->range.first.second << " to "
              << found->range.second.first << ":" << found->range.second.second << ")\x1b[0m" 
              << " in \x1b[1m" << path << "\x1b[0m" << std::endl;
    
    if (found->range.first.first != found->range.second.first) {
        throw std::runtime_error("TODO: Multiline report");
    }
    
    ReportLine(this->line(found->range.first.first), found->range.first, found->range.second);

    if (type == ParserReport::Type::PARSER_ERROR) {
        _Exit(1);
    }
}