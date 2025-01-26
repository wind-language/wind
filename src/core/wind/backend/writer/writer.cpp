/**
 * @file writer.cpp
 * @brief Implementation of the WindWriter class.
 */

#include <wind/backend/writer/writer.h>
#include <iostream>
#include <sstream>

/**
 * @brief Creates a new section in the content.
 * @param name The name of the section.
 * @return The ID of the new section.
 */
uint16_t WindWriter::Content::NewSection(std::string name) {
    sections.push_back({name, {}, ""});
    return sections.size() - 1;
}

/**
 * @brief Binds the current section to the given ID.
 * @param id The ID of the section to bind.
 */
void WindWriter::Content::BindSection(uint16_t id) { cs_id = id; }

/**
 * @brief Writes header content to the current section.
 * @param content The header content to write.
 */
void WindWriter::Content::WriteHdr(std::string content) { sections[cs_id].header += content; }

/**
 * @brief Creates a new label in the current section.
 * @param name The name of the label.
 * @return The ID of the new label.
 */
uint16_t WindWriter::Content::NewLabel(std::string name) {
    sections[cs_id].labels.push_back({name, ""});
    return sections[cs_id].labels.size() - 1;
}

/**
 * @brief Binds the current label to the given ID.
 * @param id The ID of the label to bind.
 */
void WindWriter::Content::BindLabel(uint16_t id) { cl_id = id; }

/**
 * @brief Writes content to the current label.
 * @param content The content to write.
 */
void WindWriter::Content::WriteLabel(std::string content) { sections[cs_id].labels[cl_id].content += content; }

/**
 * @brief Emits the content as a string.
 * @return The emitted content.
 */
std::string WindWriter::Emit() {
    std::string output = ".intel_syntax noprefix\n";
    for (Section &section : content.sections) {
        if (section.labels.empty() && section.header.empty()) continue;
        output += ".section " + section.name + "\n";
        output += section.header;
        if (!section.externs.empty()) output += ".globl ";
        for (std::string &ext : section.externs) {
            output += ext + ", ";
        }
        if (!section.externs.empty()) { output.pop_back(); output.pop_back(); output += "\n"; }
        for (Label &label : section.labels) {
            output += label.name + ":\n";
            //output += label.content;
            std::istringstream iss(label.content);
            for (std::string line; std::getline(iss, line);) {
                output += "  " + line + "\n";
            }
        }
    }
    return output;
}


