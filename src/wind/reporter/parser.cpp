#include <wind/reporter/parser.h>
#include <string>
#include <vector>
#include <iostream>
#include <sstream>
#include <iterator>

std::string ParserReport::line(uint16_t t_line) {
    std::string line;
    int i = 0;
    while (std::getline(this->src, line)) {
        i++;
        if (i == t_line) {
            ssize_t pos = 0;
            while ((pos = line.find('\t', pos)) != std::string::npos) {
                line.replace(pos, 1, " ");
                pos += 2;
            }
            return line;
        }
    }
    return "";
}

void ParserReport::Report(ParserReport::Type type, Token *expecting, Token *found) {
    if (type == ParserReport::Type::PARSER_WARNING) {
        std::cerr << "\x1b[33m[WARNING] \x1b[0m\x1b[1m" << "Unexpected token" << "\x1b[0m" << std::endl;
    } else if (type == ParserReport::Type::PARSER_ERROR) {
        std::cerr << "\x1b[31m[ERROR] \x1b[0m\x1b[1m" << "Unexpected token" << "\x1b[0m" << std::endl;
    }

    if (!expecting || !found) {
        return;
    }

    std::cerr << "\x1b[1m" << "Expecting: " << "\x1b[0m" << expecting->name << std::endl;
    std::cerr << "\x1b[1m" << "Found: `" << "\x1b[0m" << found->name << "\x1b[0m\x1b[36m` (" 
              << found->range.first.first << ":" << found->range.first.second << " to "
              << found->range.second.first << ":" << found->range.second.second << ")\x1b[0m" << std::endl;

    std::string start_line = this->line(found->range.first.first);
    std::string end_line = this->line(found->range.second.first);
    int start_line_length = (int)start_line.size();
    int end_line_length = (int)end_line.size();

    int start_col = found->range.first.second - 1;
    int end_col = found->range.second.second - 1;

    if (start_col > start_line_length) {
        start_col = start_line_length;
    }
    if (end_col > end_line_length) {
        end_col = end_line_length;
    }

    std::cerr << start_line << std::endl;
    for (int i = 0; i < start_line_length; i++) {
        if (i >= start_col && i < start_col + (found->range.second.second - found->range.first.second)) {
            std::cerr << "\x1b[31m" << "^" << "\x1b[0m";
        } else {
            std::cerr << " ";
        }
    }
    std::cerr << std::endl;

    if (found->range.first.first != found->range.second.first) {
        std::cerr << end_line << std::endl;
        for (int i = 0; i < end_line_length; i++) {
            if (i < end_col) {
                std::cerr << "\x1b[31m" << "^" << "\x1b[0m";
            } else {
                std::cerr << " ";
            }
        }
        std::cerr << std::endl;
    }
    exit(1);
}