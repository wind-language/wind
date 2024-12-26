#include <wind/backend/writer/writer.h>
#include <iostream>

uint8_t WindWriter::Content::NewSection(std::string name) {
    sections.push_back({name, {}, ""});
    return sections.size() - 1;
}
void WindWriter::Content::BindSection(uint8_t id) { cs_id = id; }
void WindWriter::Content::WriteHdr(std::string content) { sections[cs_id].header += content; }


uint8_t WindWriter::Content::NewLabel(std::string name) {
    sections[cs_id].labels.push_back({name, ""});
    return sections[cs_id].labels.size() - 1;
}
void WindWriter::Content::BindLabel(uint8_t id) { cl_id = id; }
void WindWriter::Content::WriteLabel(std::string content) { sections[cs_id].labels[cl_id].content += content; }


std::string WindWriter::Emit() {
    std::string output = "";
    for (Section &section : content.sections) {
        output += ".section " + section.name + ":\n";
        output += section.header;
        for (Label &label : section.labels) {
            output += label.name + ":\n";
            output += label.content;
        }
    }
    return output;
}


