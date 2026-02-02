#include <fstream> 
#include <iostream> 
#include <sstream>
#include <string>
#include <chrono>
#include <vector>
#include <map>
#include <algorithm>
#include <regex>
#include <set>
#include <iomanip>
#include <cassert>
#include <tuple>
#include <numeric>

int GLOBAL_VERTEX_COUNT = 0;
int GLOBAL_EDGES_COUNT = 0;

int GLOBAL_VERTEX_DUPE_COUNT = 0;
int GLOBAL_EDGE_DUPE_COUNT = 0;

int NF1_ADDED_NODES = 0;
int NF1_ADDED_EDGES = 0;
int NF1_ADDED_NODES_RED = 0;
int NF1_ADDED_EDGES_RED = 0;

int NF2_MOVED_ATT = 0;
int NF2_MOVED_ATT_RED = 0;

int NF3_ADDED_NODES = 0;
int NF3_ADDED_EDGES = 0;
int NF3_ADDED_NODES_RED = 0;
int NF3_ADDED_EDGES_RED = 0;

int GLOBAL_ATTRIBUTE_COUNT = 0;

std::set<std::string> GLOBAL_ATTRIBUTE_TYPES;

struct vertex {
    //typedef std::pair<int, vertex*> ve;
    std::vector<std::pair<vertex*, std::string>> labeled_adj;
    std::vector<std::pair<vertex*, std::string>> labeled_rev_adj;

    std::vector<std::tuple<vertex*, std::string, std::map<std::string, std::string>>> labeled_Adj_prop;
    bool edge_prop_used = false;

    std::vector<vertex*> adj;
    std::vector<vertex*> rev_adj;

    typedef std::map<std::string, std::string> att_map;
    typedef std::map<std::string, std::vector<std::string>> multi_att_map;

    att_map attributes;
    multi_att_map multi_attributes;

    std::string label;
    std::set<std::string> typ;
    std::string key;

    vertex(std::string s, std::string k) : label(s), key(s) {}

    void add_attribute(const std::string&, const std::string&);
    void add_type(const std::string&);
    unsigned long vertex_used_space();
};

void vertex::add_attribute(const std::string &attribute_name, const std::string& attribute_content) {
    multi_att_map::iterator mult_itr = multi_attributes.find(attribute_name);
    if (mult_itr != multi_attributes.end()) {
        multi_attributes[attribute_name].push_back(attribute_content);
    } else {
        att_map::iterator itr = attributes.find(attribute_name);
        if (itr == attributes.end()) {
            attributes[attribute_name] = attribute_content;
        } else {
            std::vector<std::string> vec;
            
            multi_attributes[attribute_name] = vec;
            multi_attributes[attribute_name].push_back(attributes[attribute_name]);
            multi_attributes[attribute_name].push_back(attribute_content);

            attributes.erase(attribute_name);
        }
    }
    if (attribute_name.compare("~label")) {
        GLOBAL_ATTRIBUTE_COUNT++;
    }
}

void vertex::add_type(const std::string& type) {
    typ.insert(type);
}

class graph
{
public:
    typedef std::map<std::string, vertex *> vmap;
    typedef std::map<std::string, std::string> alias_map;
    vmap work;
    alias_map commons;
    std::set<std::string> graph_attributes;

    void add_vertex(const std::string&, const std::string&);
    void add_vertex_regular(const std::string&, const std::string&);
    void vertex_add_attribute(const std::string&, const std::string&, const std::string&);
    void vertex_add_type(const std::string&, const std::string&);
    void add_commons(const std::string&, const std::string&);
    void add_edge(const std::string&, const std::string&, const std::string&);
    void add_edge(const std::string&, const std::string&, const std::string&, const std::map<std::string,std::string>&);
    void add_edge_GNF1(const std::string&, const std::string&, const std::string&);
    void export_to_NEO4j(const std::string&);
    void export_to_NEO4j_json(const std::string&, bool, bool, bool);
    unsigned long used_space();
};
    
unsigned long vertex::vertex_used_space() {
    unsigned long size = sizeof(attributes);
    for(typename att_map::const_iterator it = attributes.begin(); it != attributes.end(); ++it){
        size += it->first.size();
        size += it->second.size();
    }
    for(typename multi_att_map::const_iterator it = multi_attributes.begin(); it != multi_attributes.end(); ++it){
        size += it->first.size();
        size += it->second.size() * sizeof(std::string) ;
        size += sizeof(std::vector<std::string>);
    }
    for(auto ele : labeled_adj){
        size += sizeof(vertex*) + sizeof(std::pair<vertex*, std::string>) + ele.second.size();
    }
    return size;
}

unsigned long graph::used_space() {
    //int ret = ((sizeof(std::string))* work.size())+sizeof(std::map<std::string, vertex *>);
    unsigned long size = sizeof(work);
    for(typename vmap::const_iterator it = work.begin(); it != work.end(); ++it){
        size += it->first.size();
        size += it->second->vertex_used_space();
    }
    return size;
}

void graph::add_vertex(const std::string &resource, const std::string &label) {
    vmap::iterator itr = work.find(resource);
    if (itr == work.end()) {
        GLOBAL_ATTRIBUTE_COUNT++;
        vertex *v;
        v = new vertex(label, resource);
        work[resource] = v;
        GLOBAL_VERTEX_COUNT++;
        return;
    }
}

void graph::add_vertex_regular(const std::string &resource, const std::string &label) {
    vmap::iterator itr = work.find(resource);
    if (itr == work.end()) {
        vertex *v;
        v = new vertex(label, resource);
        work[resource] = v;
        GLOBAL_ATTRIBUTE_COUNT++;
        GLOBAL_VERTEX_COUNT++;
        return;
    }
    GLOBAL_VERTEX_DUPE_COUNT++;
}

void graph::add_commons(const std::string &resource, const std::string &common) {
    add_vertex(resource, resource);

    commons[common] = resource;
}

void graph::vertex_add_attribute(const std::string &resource, const std::string &attribute_name, const std::string& attribute_content) {
    alias_map::iterator itr = commons.find(resource);
    std::string vertex_id = resource;
    if (itr != commons.end()) {
        vertex_id = commons[resource];
    }

    add_vertex(vertex_id, vertex_id);
    graph_attributes.insert(attribute_name);
    work[vertex_id]->add_attribute(attribute_name, attribute_content);
}

void graph::vertex_add_type(const std::string &resource, const std::string &type) {
    alias_map::iterator itr = commons.find(resource);
    std::string vertex_id = resource;
    if (itr != commons.end()) {
        vertex_id = commons[resource];
    }

    add_vertex(vertex_id, vertex_id);

    work[vertex_id]->add_type(type);
}

void graph::add_edge(const std::string &resource, const std::string &attribute_name, const std::string& destination) {
    alias_map::iterator itr = commons.find(resource);
    std::string vertex_id_source = resource;
    if (itr != commons.end()) {
        vertex_id_source = commons[resource];
    }

    add_vertex(vertex_id_source, vertex_id_source);

    std::string vertex_id_destination = destination;
    itr = commons.find(destination);
    if (itr != commons.end()) {
        vertex_id_destination = commons[destination];
    }

    add_vertex(vertex_id_destination, vertex_id_destination);

    vertex *f = (work.find(vertex_id_source)->second);
    vertex *t = (work.find(vertex_id_destination)->second);

    std::pair<vertex*, std::string> source_pair(f, attribute_name);
    std::pair<vertex*, std::string> destination_pair(t, attribute_name);

    if(std::find(f->labeled_adj.begin(), f->labeled_adj.end(), destination_pair) == f->labeled_adj.end()) {
        f->adj.push_back(t);
        f->labeled_adj.push_back(destination_pair);

        t->rev_adj.push_back(f);
        t->labeled_rev_adj.push_back(source_pair);

        GLOBAL_EDGES_COUNT++;
    } else {
        GLOBAL_EDGE_DUPE_COUNT++;
    }
}

void graph::add_edge(const std::string &resource, const std::string &attribute_name, const std::string& destination, const std::map<std::string,std::string>& properties) {
    alias_map::iterator itr = commons.find(resource);
    std::string vertex_id_source = resource;
    if (itr != commons.end()) {
        vertex_id_source = commons[resource];
    }

    add_vertex(vertex_id_source, vertex_id_source);

    std::string vertex_id_destination = destination;
    itr = commons.find(destination);
    if (itr != commons.end()) {
        vertex_id_destination = commons[destination];
    }

    add_vertex(vertex_id_destination, vertex_id_destination);

    vertex *f = (work.find(vertex_id_source)->second);
    vertex *t = (work.find(vertex_id_destination)->second);

    std::pair<vertex*, std::string> source_pair(f, attribute_name);
    std::pair<vertex*, std::string> destination_pair(t, attribute_name);

    std::tuple<vertex*, std::string, std::map<std::string,std::string>> edge_triple(t, attribute_name, properties);

    if(std::find(f->labeled_adj.begin(), f->labeled_adj.end(), destination_pair) == f->labeled_adj.end()) {
        f->adj.push_back(t);
        f->labeled_adj.push_back(destination_pair);
        f->labeled_Adj_prop.push_back(edge_triple);

        t->rev_adj.push_back(f);
        t->labeled_rev_adj.push_back(source_pair);

        GLOBAL_EDGES_COUNT++;
    } else {
        GLOBAL_EDGE_DUPE_COUNT++;
    }
}

void graph::add_edge_GNF1(const std::string &resource, const std::string &attribute_name, const std::string& destination) {
    GLOBAL_ATTRIBUTE_COUNT--;
    alias_map::iterator itr = commons.find(resource);
    std::string vertex_id_source = resource;
    if (itr != commons.end()) {
        vertex_id_source = commons[resource];
    }

    std::string vertex_id_destination = destination;
    if (itr != commons.end()) {
        vertex_id_destination = commons[destination];
    }
    vmap::iterator itr2 = work.find(vertex_id_destination);
    if (itr2 == work.end()) {
        add_vertex_regular(vertex_id_destination, vertex_id_destination);
        vertex_add_attribute(vertex_id_destination, "~label", attribute_name);
    } else {
        add_vertex_regular(vertex_id_destination, vertex_id_destination);
    }
    

    vertex *f = (work.find(vertex_id_source)->second);
    vertex *t = (work.find(vertex_id_destination)->second);

    std::pair<vertex*, std::string> source_pair(f, attribute_name);
    std::pair<vertex*, std::string> destination_pair(t, attribute_name);

    if(std::find(f->labeled_adj.begin(), f->labeled_adj.end(), destination_pair) == f->labeled_adj.end()) {
        f->adj.push_back(t);
        f->labeled_adj.push_back(destination_pair);

        t->rev_adj.push_back(f);
        t->labeled_rev_adj.push_back(source_pair);

        GLOBAL_EDGES_COUNT++;
    } else {
        GLOBAL_EDGE_DUPE_COUNT++;
        GLOBAL_VERTEX_DUPE_COUNT--;
    }
}
// Function to convert a character to its stringified Unicode representation
std::string unicodeToString(unsigned char ch) {
    std::ostringstream oss;
    oss << "u" << std::hex << std::setw(4) << std::setfill('0') << static_cast<int>(ch);
    //oss << "u" <<  static_cast<int>(ch);
    return oss.str();
}

// Function to replace Unicode characters in the specified range with their stringified version
std::string replaceUnicodeWithStrings(const std::string& input) {
    std::string result;
    for (char ch : input) {
        
        unsigned char uch = static_cast<unsigned char>(ch);
        if ((uch >= 0x80/* && uch <= 0xFF*/) || uch == 0x2a) {
            std::string tmp = unicodeToString(uch);
            tmp.insert(tmp.find('0')+2, 1, ' ');
            //if (result.size() != 0) {result.pop_back();}
            result += tmp;
        } else if (result.size()!=0 && result.back()=='\\' && uch == 'u') {
            result.pop_back();
        } else {
            result += ch;
        }
    }
    return result;
}

std::string extract_label(std::string text) {
    std::string::size_type start_position = 0;
    std::string::size_type end_position = text.length()-3;
    std::string found_text;

    start_position = text.find("\"");
    std::string::size_type last_before = std::string::npos;
    if (start_position != std::string::npos) {
        ++start_position; // start after the double quotes.
        std::string::size_type offset = start_position;
        // look for end position;
        end_position = text.find("\"", offset);
        while(end_position != std::string::npos) {
            last_before = end_position;
            ++offset; // start after the double quotes.
            // look for end position;
            end_position = text.find("\"", offset);
        }
        
        if (last_before != std::string::npos) {
            found_text = text.substr(start_position, last_before - start_position);
            found_text = replaceUnicodeWithStrings(found_text);
            // std::regex e ("[\u0080-\u009F]");
            // if (std::regex_search(found_text, e)) {
            //     std::cout << found_text << std::endl;
            //     found_text = replaceUnicodeWithStrings(found_text);
            //     std::cout << "found" << std::endl;
            //     std::cout << found_text << std::endl;
            // }
            // size_t pos = found_text.find("\u0085"); 
            // while (pos != std::string::npos) { 
            //     found_text = "\\u0085";
            //     std::regex_search()
            //     // Find the next occurrence of the substring 
            //     pos = found_text.find("\u0085", pos); 
            // }
            // if (found_text.find("U+00") != std::string::npos) {
            //     // std::cout << found_text << std::endl;
            //     // std::cout << "found" << std::endl;
            //     found_text.insert(found_text.find('0')+2, 1, '\\');
            //     // std::cout << found_text << std::endl;
            // }
            return found_text;
        }
    }
    return "";
}

std::string extract_year(std::string text) {
    std::string::size_type start_position = 0;
    std::string::size_type end_position = text.length()-3;
    std::string found_text;

    start_position = text.find("\"");
    if (start_position != std::string::npos) {
        ++start_position; // start after the double quotes.
        // look for end position;
        end_position = text.find("\"", start_position);
        if (end_position != std::string::npos) {
            found_text = text.substr(start_position, end_position - start_position);
            return found_text;
        }
    }
    return "-1";
    
}

std::string extract_brackets(std::string text, int pos=0) {
    std::string::size_type start_position = 0;
    std::string::size_type end_position = text.length()-3;
    std::string found_text;

    int pointer = 1;
    start_position = text.find("<");

    while ((start_position != std::string::npos) && (pointer < pos)) {
        start_position++;
        start_position = text.find("<", start_position);
        pointer++;
    }

    if (start_position != std::string::npos) {
        ++start_position; // start after the double quotes.
        // look for end position;
        end_position = text.find(">", start_position);
        if (end_position != std::string::npos) {
            found_text = text.substr(start_position, end_position - start_position);
            found_text = replaceUnicodeWithStrings(found_text);
            return found_text;
        }
    }
    return "";
}

void graph::export_to_NEO4j(const std::string& filename) {
    std::ofstream myfile;
    std::string prefix = "";//"node:";
    std::string prefix_att = "";//"attribute:";
    myfile.open (filename+"_nodes.ttl");
    for (const auto& node : work) {
        std::string tmp = "";
        tmp = "<" + prefix + node.first + "> " + prefix_att + "<label> \"" + node.second->label + "\" ;";
        for (auto& att : node.second->attributes) {
            tmp += "\n    <" + prefix_att + att.first + "> \"" + att.second + "\" ;";
        }
        for (auto& mult_att : node.second->multi_attributes) {
            for (auto& att : mult_att.second) {
                tmp += "\n    <" + prefix_att + mult_att.first + "> \"" + att + "\" ;";
            }
        }
        for (auto& edge : node.second->labeled_adj) {
            std::string s = prefix + edge.first->key;
            std::replace( s.begin(), s.end(), ' ', '_');
            tmp += "\n    <" + prefix_att + edge.second + "> <" + s + "> ;";
        }
        if (!tmp.empty()){
            tmp.pop_back();
        }
        myfile << tmp << ".\n";
    }
    myfile.close();
}

void graph::export_to_NEO4j_json(const std::string& filename, bool quotemarks=true, bool northwind_label=false, bool offshore_label=false) {
    std::ofstream myfile;
    std::map<std::string, int> vertex_id_map;
    myfile.open ("results/"+filename+"_nodes.json");
    int id_counter = 0;
    for (const auto& node : work) {
        std::string tmp = "{\"type\":\"node\",\"id\":\"" + std::to_string(id_counter) + "\",\"labels\":[";
        tmp += "\"NODE\"";
        // if (node.second->attributes.find("~label") != node.second->attributes.end() ) {
        //     tmp += ",\""+node.second->attributes["~label"]+"\"";
        // }
        // else if (node.second->attributes.find("\"~label\"") != node.second->attributes.end() ) {
        //     tmp += "," + node.second->attributes["\"~label\""];
        // } //else {
        //     tmp += "\"NODE\"";
        // }
        // else if (offshore_label) {
        //     bool first_flag = true;
        //     for (const auto& att : node.second->typ) {
        //         if (!first_flag) {
        //             tmp += ",";
        //         }
        //         tmp += att + "";
        //         first_flag = false;
        //     }
        // }
        // else {
        //     tmp += "\"NODE\"";
        // }
        if (quotemarks) {
            tmp += "],\"properties\":{\"IRI\":\"" + node.first +"\",\"label_name\":\"" + node.second->label +"\"";
        } else {
            tmp += "],\"properties\":{\"IRI\":" + node.first +",\"label_name\":" + node.second->label;
        }
        
        for (const auto& att : node.second->attributes) {
            if (quotemarks) {
                tmp += ",\"" + att.first + "\":\"" + att.second + "\"";
            } else {
                tmp += "," + att.first + ":" + att.second;
            }
            
        }
        if ((node.second->attributes.find("~label") == node.second->attributes.end()) && (node.second->attributes.find("\"~label\"") == node.second->attributes.end()) && (node.second->multi_attributes.find("\"~label\"") == node.second->multi_attributes.end()) && (node.second->multi_attributes.find("~label") == node.second->multi_attributes.end()) ) {
            tmp += ",\"~label\":\"NODE\"";
        }
        for (const auto& matt : node.second->multi_attributes) {
            tmp += ",\"" + matt.first + "\":[";
            bool is_first = true;
            for (const auto& att : matt.second) {
                if (!is_first) {
                    tmp += ",";
                }
                if (quotemarks) {
                    tmp += "\"" + att + "\"";
                } else {
                    tmp += "" + att + "";
                }
                
                is_first = false;            }
            tmp += "]";
        }
        myfile << tmp << "}}\n";
        vertex_id_map[node.second->label] = id_counter;
        id_counter++;
    }
    myfile.close();
    // {"id":"0","type":"relationship","label":"KNOWS","properties":{"bffSince":"P5M1DT12H","since":1993},"start":{"id":"0","labels":["User"]},"end":{"id":"1","labels":["User"],"properties":{"name":"Jim","age":42}}}
    myfile.open ("results/"+filename+"_edges.json");
    int edge_id_counter = id_counter;// 0;
    for (const auto& node : work) {
        for (const auto& edge : node.second->labeled_adj) {
            std::string tmp;
            if (quotemarks) {
                tmp = "{\"id\":\"" + std::to_string(edge_id_counter) + "\",\"type\":\"relationship\",\"label\":\"" + edge.second  + "\",\"properties\":{},\"start\":{\"id\":\"" + std::to_string(vertex_id_map[node.second->label]) +"\",\"labels\":[\"NODE\"]},\"end\":{\"id\":\"" + std::to_string(vertex_id_map[edge.first->label]) +"\",\"labels\":[\"NODE\"]}}\n";
                // if ((node.second->attributes.find("~label") != node.second->attributes.end() ) && (edge.first->attributes.find("~label") != edge.first->attributes.end() )) {
                //     tmp = "{\"id\":\"" + std::to_string(edge_id_counter) + "\",\"type\":\"relationship\",\"label\":\"" + edge.second  + "\",\"properties\":{},\"start\":{\"id\":\"" + std::to_string(vertex_id_map[node.second->label]) +"\",\"labels\":[\""+node.second->attributes["~label"]+"\"]},\"end\":{\"id\":\"" + std::to_string(vertex_id_map[edge.first->label]) +"\",\"labels\":[\""+edge.first->attributes["~label"]+"\"]}}\n";
                // } else {
                //     tmp = "{\"id\":\"" + std::to_string(edge_id_counter) + "\",\"type\":\"relationship\",\"label\":\"" + edge.second  + "\",\"properties\":{},\"start\":{\"id\":\"" + std::to_string(vertex_id_map[node.second->label]) +"\",\"labels\":[\"NODE\"]},\"end\":{\"id\":\"" + std::to_string(vertex_id_map[edge.first->label]) +"\",\"labels\":[\"NODE\"]}}\n";
                // }
            } else {
                tmp = "{\"id\":\"" + std::to_string(edge_id_counter) + "\",\"type\":\"relationship\",\"label\":" + edge.second  + ",\"properties\":{},\"start\":{\"id\":\"" + std::to_string(vertex_id_map[node.second->label]) +"\",\"labels\":[\"NODE\"]},\"end\":{\"id\":\"" + std::to_string(vertex_id_map[edge.first->label]) +"\",\"labels\":[\"NODE\"]}}\n";
                // if ((node.second->attributes.find("\"~label\"") != node.second->attributes.end() ) && (edge.first->attributes.find("\"~label\"") != edge.first->attributes.end() )) {
                //     tmp = "{\"id\":\"" + std::to_string(edge_id_counter) + "\",\"type\":\"relationship\",\"label\":" + edge.second  + ",\"properties\":{},\"start\":{\"id\":\"" + std::to_string(vertex_id_map[node.second->label]) +"\",\"labels\":["+node.second->attributes["\"~label\""]+"]},\"end\":{\"id\":\"" + std::to_string(vertex_id_map[edge.first->label]) +"\",\"labels\":["+edge.first->attributes["\"~label\""]+"]}}\n";
                // } else {
                //     tmp = "{\"id\":\"" + std::to_string(edge_id_counter) + "\",\"type\":\"relationship\",\"label\":" + edge.second  + ",\"properties\":{},\"start\":{\"id\":\"" + std::to_string(vertex_id_map[node.second->label]) +"\",\"labels\":[\"NODE\"]},\"end\":{\"id\":\"" + std::to_string(vertex_id_map[edge.first->label]) +"\",\"labels\":[\"NODE\"]}}\n";
                // }
            }
            myfile << tmp;
            edge_id_counter++;
        }
    }
    myfile.close();
}

int removeSubstrs(std::string& s, std::string& p) { 
    std::string::size_type n = p.length();
    for (std::string::size_type i = s.find(p); i != std::string::npos; i = s.find(p)) {
        s.erase(i, n);
    }
    return 0;
}

void readDBpedia(graph& G) {

    std::string path="datasets/dbpedia/", file_label = path + "labels_lang=en.ttl"; 
    std::ifstream inputFile(file_label); 
    std::string resource, label, line, word;

    while (getline(inputFile, line)) {
        
        std::stringstream s(line);
        int position = 0;

        resource = extract_brackets(line, 1);
        label = extract_label(line);

        G.add_vertex_regular(resource, label);

        assert(extract_brackets(line,2).compare("http://www.w3.org/2000/01/rdf-schema#label")==0);
    }
    inputFile.close();
    std::string file_commons = path + "commons-sameas-links_lang=en.ttl";
    inputFile.open(file_commons);

    std::string commons;

    while (getline(inputFile, line)) {
        
        std::stringstream s(line);
        int position = 0;

        resource = extract_brackets(line, 1);
        commons = extract_brackets(line, 3);

        G.add_commons(resource, commons);

        assert(extract_brackets(line,2).compare("http://www.w3.org/2002/07/owl#sameAs")==0);
    }
    
    inputFile.close();
    std::string file_types = path + "instance-types_lang=en_specific.ttl";
    inputFile.open(file_types);

    std::string type;

    while (getline(inputFile, line)) {
        
        std::stringstream s(line);
        int position = 0;

        resource = extract_brackets(line, 1);
        type = extract_brackets(line, 3);

        G.vertex_add_type(resource, type);
        type = ":" + type;
        G.vertex_add_attribute(resource, "~label", type);

        assert(extract_brackets(line,2).compare("http://www.w3.org/1999/02/22-rdf-syntax-ns#type")==0);
    }
    
    inputFile.close();

    std::string file_literals = path + "mappingbased-literals_lang=en.ttl";
    inputFile.open(file_literals);
    std::string attribute_name, attribute_content;

    while (getline(inputFile, line)) {
        
        std::stringstream s(line);
        int position = 0;

        resource = extract_brackets(line, 1);
        attribute_name = extract_brackets(line, 2);
        attribute_content = extract_label(line);

        GLOBAL_ATTRIBUTE_TYPES.insert(attribute_name);

        G.vertex_add_attribute(resource, attribute_name, attribute_content);
        //GLOBAL_ATTRIBUTE_COUNT++;

        assert((line.find("\"") != std::string::npos));
    }

    inputFile.close();
    std::string file_infobox = path + "infobox-properties_lang=en.ttl";
    inputFile.open(file_infobox);

    while (getline(inputFile, line)) {

        std::stringstream s(line);
        int position = 0;

        resource = extract_brackets(line, 1);
        attribute_name = extract_brackets(line, 2);

        if (line.find("\"") != std::string::npos) {
            attribute_content = extract_label(line);
            G.vertex_add_attribute(resource, attribute_name, attribute_content);
            //GLOBAL_ATTRIBUTE_COUNT++;
            GLOBAL_ATTRIBUTE_TYPES.insert(attribute_name);
        } else {
            attribute_content = extract_brackets(line, 3);
            G.add_edge(resource, attribute_name, attribute_content);
        }
        if (resource.compare("http://dbpedia.org/resource/%22Good_day,_fellow!%22_%22Axe_handle!%22")==0) {
            std::cout << std::endl;
            std::cout << resource << std::endl;
            std::cout << attribute_content << std::endl;
        }
    }
    inputFile.close();
    std::string file_objects = path + "mappingbased-objects_lang=en.ttl";
    inputFile.open(file_objects);

    while (getline(inputFile, line)) {
        
        std::stringstream s(line);
        int position = 0;

        resource = extract_brackets(line, 1);
        attribute_name = extract_brackets(line, 2);
        attribute_content = extract_brackets(line, 3);
        G.add_edge(resource, attribute_name, attribute_content);
        
        assert((line.find("\"") == std::string::npos));
    }
    inputFile.close();
}

void readDBLP(graph& G) {
    std::string filename = "datasets/dblp/dblp-2024-03-01.nt"; 
    // std::string filename = "datasets/dblp/test.nt"; 
    std::ifstream inputFile(filename); 

    if (!inputFile.is_open()) { 
        std::cerr << "Error opening file: " << filename << std::endl;
    } 
  
    std::string line;
    std::string resource;
    bool print_now = false;
    vertex::att_map attributes;
    vertex::multi_att_map mult_attributes;
    std::vector<std::string> start;
    mult_attributes["affiliations"] = start;
    bool printnow = false;
    int count = 0;
    while (getline(inputFile, line)) {
        //std::cout << line << std::endl;
        if (line.find("provenance information for the dblp RDF dump") != std::string::npos) {
            break;
        }
        if (print_now) {std::cout << line << std::endl;}
        if (line.empty()) {
            G.add_vertex(resource, resource);
            for (const auto& at : attributes) {
                G.vertex_add_attribute(resource, at.first, at.second);
            }
            for (const auto& mat : mult_attributes) {
                for (const auto& at : mat.second) {
                    G.vertex_add_attribute(resource, mat.first, at);
                }
            }
            count++;
            printnow = true;
            resource = "";
            attributes.clear();
            mult_attributes["affiliations"].clear();
        }
        if (line.find("schema#label") != std::string::npos) {
            resource = extract_label(line);
        }
        if (line.find("https://doi.org/") != std::string::npos) {
            GLOBAL_ATTRIBUTE_COUNT++;
            attributes["doi"] = extract_brackets(line, 3);
        }
        if (line.find("schema#primaryCreatorName") != std::string::npos) {
            GLOBAL_ATTRIBUTE_COUNT++;
            attributes["name"] = extract_label(line);
        }
        if (line.find("schema#title") != std::string::npos) {
            GLOBAL_ATTRIBUTE_COUNT++;
            attributes["title"] = extract_label(line);
        }
        if (line.find("schema#yearOfPublication") != std::string::npos) {
            GLOBAL_ATTRIBUTE_COUNT++;
            attributes["year"] = extract_year(line);
        }
        if (line.find("schema#publishedIn>") != std::string::npos) {
            GLOBAL_ATTRIBUTE_COUNT++;
            attributes["publishedIn"] = extract_label(line);
        }
        if (line.find("schema#affiliation") != std::string::npos) {
            GLOBAL_ATTRIBUTE_COUNT++;
            mult_attributes["affiliations"].push_back(extract_label(line));
        }
        if (printnow && count%100000==0) {
            std::cout << count << std::endl;
            printnow = false;
        }
    }
    //inputFile.close();
    //std::cout << "Now the edges!" << std::endl;
    inputFile.clear();
    inputFile.seekg (0, std::ios::beg);
    if (!inputFile.is_open()) { 
        std::cerr << "Error opening file: " << filename << std::endl; 
    } 
    std::string label;
    std::string creator;
    int edge_count = 0;
    while (getline(inputFile, line)) {
        //std::cout << line << std::endl;
        if (line.find("provenance information for the dblp RDF dump") != std::string::npos) {
            break;
        }
        if (line.empty()) {
            label = "UNKNOWN FLAG";
            creator = "";
        }
        if (line.find("schema#label") != std::string::npos) {
            label = extract_label(line);
        }
        if (line.find("schema#signatureDblpName") != std::string::npos) {
            creator = extract_label(line);

            G.add_edge(creator, "written", label);
            edge_count++;
        }
    }
}

void readDBLP_full(graph& G) {
    std::string filename = "datasets/dblp/dblp-2024-03-01.nt"; 
    // std::string filename = "datasets/dblp/test.nt"; 
    std::ifstream inputFile(filename); 

    if (!inputFile.is_open()) { 
        std::cerr << "Error opening file: " << filename << std::endl;
    } 
  
    std::string line;
    auto start_load = std::chrono::high_resolution_clock::now();
    std::string resource,attribute_name,attribute_content;
    bool new_block = true;
    bool printnow = false;
    int count = 0;
    while (getline(inputFile, line)) {
        //std::cout << line << std::endl;
        if (line.find("provenance information for the dblp RDF dump") != std::string::npos) {
            break;
        }
        
        resource = extract_brackets(line, 1);
        if (new_block) {
            count++;
            printnow = true;
            G.add_vertex(resource, resource);
            new_block = false;
        }
        attribute_name = extract_brackets(line, 2);
        if (line.find("_:ID")  != std::string::npos || line.find("_:Sig")  != std::string::npos) {
            if (line[0] == '_') {
                resource = line.substr(0,line.find('<')-1);
                resource = std::regex_replace(resource, std::regex("^ +| +$|( ) +"), "$1");
                attribute_name = extract_brackets(line, 1);
                if (line.find("\"") != std::string::npos) {
                    attribute_content = extract_label(line);
                    G.vertex_add_attribute(resource, attribute_name, attribute_content);
                    GLOBAL_ATTRIBUTE_TYPES.insert(attribute_name);
                } else {
                    attribute_content = extract_brackets(line, 2);
                    G.add_edge(resource, attribute_name, attribute_content);
                }
            } else {
                std::string marker = "_:ID";
                if (line.find("_:Sig")  != std::string::npos) {
                    marker = "_:Sig";
                } 
                resource = extract_brackets(line, 1);
                attribute_name = extract_brackets(line, 2);
                attribute_content = line.substr(line.find(marker),line.length());
                attribute_content.pop_back();
                attribute_content = std::regex_replace(attribute_content, std::regex("^ +| +$|( ) +"), "$1");
                G.add_edge(resource, attribute_name, attribute_content);
            }
        } else if (line.find("\"") != std::string::npos) {
            attribute_content = extract_label(line);
            G.vertex_add_attribute(resource, attribute_name, attribute_content);
            GLOBAL_ATTRIBUTE_TYPES.insert(attribute_name);
        } else {
            attribute_content = extract_brackets(line, 3);
            G.add_edge(resource, attribute_name, attribute_content);
        }

        if (line.empty()) {
            new_block = true;
        }
        if (printnow && count%100000==0) {
            std::cout << count << std::endl;
            printnow = false;
        }
    }
    inputFile.clear();
}

void readDBLP_paper_author(graph& G) {
    std::string filename = "datasets/dblp/dblp-2024-03-01.nt"; 
    // std::string filename = "datasets/dblp/test.nt"; 
    std::ifstream inputFile(filename); 

    if (!inputFile.is_open()) { 
        std::cerr << "Error opening file: " << filename << std::endl;
    } 
  
    std::string line;
    auto start_load = std::chrono::high_resolution_clock::now();
    std::string resource,attribute_name,attribute_content;
    bool new_block = true;
    bool printnow = false;
    int count = 0;
    while (getline(inputFile, line)) {
        //std::cout << line << std::endl;
        if (line.find("provenance information for the dblp RDF dump") != std::string::npos) {
            break;
        }
        
        if (new_block) {
            resource = extract_brackets(line, 1);
            count++;
            printnow = true;
            if (resource.compare("") != 0) {
                G.add_vertex(resource, resource);
                new_block = false;
            }
        }
        attribute_name = extract_brackets(line, 2);
        if (line.find("_:ID")  != std::string::npos || line.find("_:Sig")  != std::string::npos) {
            continue;
        } else if (line.find("https://dblp.org/rdf/schema#authoredBy") != std::string::npos) {
            attribute_name = extract_brackets(line, 2);
            attribute_content = extract_brackets(line, 3);
            G.add_edge(resource, attribute_name, attribute_content);
            continue;
        } else if (line.find("\"") != std::string::npos) {
            attribute_content = extract_label(line);
        } else {
            attribute_content = extract_brackets(line, 3);
        }
        if ((resource.compare("") != 0) && (attribute_name.compare("") != 0) && (attribute_content.compare("") != 0)) {
            if ( (attribute_content.compare("https://dblp.org/rdf/schema#Publication") == 0) || (attribute_content.compare("https://dblp.org/rdf/schema#Person") == 0) || (attribute_content.compare("https://dblp.org/rdf/schema#AmbiguousCreator") == 0) || (attribute_content.compare("https://dblp.org/rdf/schema#Group") == 0) ) {
                attribute_content = ":" + attribute_content;
                G.vertex_add_attribute(resource, "~label", attribute_content);
            } else {
                G.vertex_add_attribute(resource, attribute_name, attribute_content);
                GLOBAL_ATTRIBUTE_TYPES.insert(attribute_name);
            }
            
        }
        

        if (line.empty()) {
            new_block = true;
        }
        // if (printnow && count%100000==0) {
        //     std::cout << resource << std::endl;
        //     std::cout << count << std::endl;
        //     printnow = false;
        // }
    }
    inputFile.clear();
}

void readGO(graph& G) {
    std::string filename = "datasets/go/go.nt"; 
    std::ifstream inputFile(filename); 

    if (!inputFile.is_open()) { 
        std::cerr << "Error opening file: " << filename << std::endl;
    } 
  
    std::string line;
    auto start_load = std::chrono::high_resolution_clock::now();
    std::string resource,attribute_name,attribute_content;
    bool new_block = false;
    while (getline(inputFile, line)) {
        //std::cout << line << std::endl;
        if (line.find("BREAK")  != std::string::npos) {
            new_block = true;
            continue;
        }
        resource = extract_brackets(line, 1);
        attribute_name = extract_brackets(line, 2);
        if (line.find("\"") != std::string::npos) {
            attribute_content = extract_label(line);
            if (attribute_name.compare("http://www.geneontology.org/formats/oboInOwl#hasOBONamespace") == 0) {
                G.vertex_add_attribute(resource, "~label", ":"+attribute_content);
            } else {
                G.vertex_add_attribute(resource, attribute_name, attribute_content);
            }
            GLOBAL_ATTRIBUTE_TYPES.insert(attribute_name);
        } else {
            attribute_content = extract_brackets(line, 3);
            if (!new_block) {
                NF1_ADDED_EDGES++;
                graph::vmap::iterator itr = G.work.find(attribute_content);
                if (itr == G.work.end()) {
                    NF1_ADDED_NODES++;
                }
            }
            G.add_edge(resource, attribute_name, attribute_content);
            
        }
    }
    inputFile.clear();
}

void readMimic(graph& G) {
    std::string filename = "datasets/mimic/mimic.nt"; 
    std::ifstream inputFile(filename); 

    if (!inputFile.is_open()) { 
        std::cerr << "Error opening file: " << filename << std::endl;
    } 
  
    std::string line;
    auto start_load = std::chrono::high_resolution_clock::now();
    std::string resource,attribute_name,attribute_content;
    while (getline(inputFile, line)) {
        resource = extract_brackets(line, 1);
        attribute_name = extract_brackets(line, 2);

        if (line.find("\"") != std::string::npos) {
            attribute_content = extract_label(line);
            G.vertex_add_attribute(resource, attribute_name, attribute_content);
            GLOBAL_ATTRIBUTE_TYPES.insert(attribute_name);
        } else {
            attribute_content = extract_brackets(line, 3);
            G.add_edge(resource, attribute_name, attribute_content);
            
        }
    }
    inputFile.clear();
}

void readOffshore(graph& G) {
    std::string file_label = "datasets/offshore/offshore.csv";
    std::ifstream inputFile(file_label); 
  
    std::string line, word;
    bool print_now = false;

    std::string resource, label, tmp, source, dest, edge_lab;
    int missing=0,edge_count = 0;
    int column_count = 0;
    bool header_line = true, stored = false, edge_starter = false;
    std::vector<std::string> header;
    int position = 0;
    int marks = 0;
    while (getline(inputFile, line)) {
        
        std::stringstream s(line);
        vertex::att_map attr;
        while (getline(s, word, ',')) { 
            if (header_line) {
                header.push_back(word);
                position++;
                column_count = position;
            } else {
                marks += std::count(word.begin(), word.end(), '\"');
                if (!stored && (marks%2 == 1)) {
                    tmp = word;
                    stored = true;
                } else if (stored && (marks%2 == 0)) {
                    tmp += word;
                    stored = false;
                } else if (stored) {
                    tmp += word;
                } else {
                    tmp = word;
                }
                if (!stored) {
                    //std::cout << (header.at(position).compare("_id") == 0) << "  " << (tmp.compare("") != 0) << "  " << (tmp.compare("\"\"") != 0) << std::endl;
                    if ((header.at(position).compare("\"_id\"") == 0) && (tmp.compare("") != 0) && (tmp.compare("\"\"") != 0)) {
                        resource = tmp;
                    } else if ((header.at(position).compare("\"_labels\"") == 0) && (tmp.compare("") != 0) && (tmp.compare("\"\"") != 0)) {
                        label = tmp;
                    } else if ((header.at(position).compare("\"_start\"") == 0) && (tmp.compare("") != 0) && (tmp.compare("\"\"") != 0)) {
                        edge_starter = true;
                        source = tmp;
                    } else if ((header.at(position).compare("\"_end\"") == 0) && (tmp.compare("") != 0) && (tmp.compare("\"\"") != 0)) {
                        dest = tmp;
                    } else if ((header.at(position).compare("\"_type\"") == 0) && (tmp.compare("") != 0) && (tmp.compare("\"\"") != 0)) {
                        edge_lab = tmp;
                    }else if ((tmp.compare("") != 0) && (tmp.compare("\"\"") != 0)){
                        //std::cout << tmp << std::endl;
                        tmp.erase(remove(tmp.begin(), tmp.end(), '\"'),tmp.end());
                        tmp.erase(remove(tmp.begin(), tmp.end(), '\t'),tmp.end());
                        tmp.erase(remove(tmp.begin(), tmp.end(), '\\'),tmp.end());
                        tmp.erase(remove(tmp.begin(), tmp.end(), '^'),tmp.end());
                        std::string filler = header[position];
                        filler.erase(remove(filler.begin(), filler.end(), '\"'),filler.end());
                        filler.erase(remove(filler.begin(), filler.end(), '\t'),filler.end());
                        filler.erase(remove(filler.begin(), filler.end(), '\\'),filler.end());
                        filler.erase(remove(filler.begin(), filler.end(), '^'),filler.end());
                        attr[header[position]] = "\""+ filler + "_" + tmp + "\"";
                        //attr[header[position]] = "\"" + tmp + "\"";
                    }
                    position++;
                }
                
                //std::cout << position << std::endl;
            }
            
        }
        //std::cout << std::endl;
        if (header_line) {
            header_line = false;
            position = 0;
        } else if (position==column_count || position==column_count-1) {
            marks = 0;
            position = 0;
            if (edge_starter) {
                G.add_edge(source, edge_lab, dest);
            } else {
                G.add_vertex(resource, resource);
                G.vertex_add_attribute(resource, "\"~label\"", label);
                for (const auto& at : attr) {
                    G.vertex_add_attribute(resource, at.first, at.second);
                }
            }
        }
        assert(position <= column_count);
    }
    inputFile.close();
}

void readNorthwind(graph& G) {

    
    std::string filename = "datasets/northwind/northwind_nodes.csv";
    std::fstream fin; 
    fin.open(filename, std::ios::in);

    std::string line, word;
    std::vector<std::string> header;
    std::vector<std::string> header_edge;
    int count = 0, column_count = 0, position;
    bool header_line = true;

    while (getline(fin, line)) {
  
        std::stringstream s(line);

        position = 0;
        
        std::string id, type;

        while (getline(s, word, ',')) {
            if (header_line) {
                word.erase(remove(word.begin(), word.end(), '\r'),word.end());
                word.erase(remove(word.begin(), word.end(), '\n'),word.end());
                header.push_back(word);
                column_count = position;
            } else {
                count++;
                if (position == 0) {
                    id = "ID_" + word;
                    G.add_vertex(id, id);
                } else {
                    if (word.compare("") != 0 && word.compare(" ") != 0 && word.compare("NULL") != 0 && word.compare("\r") != 0 && word.compare("\n") != 0) {
                        word.erase(remove(word.begin(), word.end(), '\r'),word.end());
                        word.erase(remove(word.begin(), word.end(), '\n'),word.end());
                        word.erase(remove(word.begin(), word.end(), '\"'),word.end());
                        word.erase(remove(word.begin(), word.end(), '\''),word.end());
                        G.vertex_add_attribute(id, header[position], word);
                    }
                }
            }
            position++;
        }
        header_line = false;
    }

    std::string filename_edges = "datasets/northwind/northwind_edges.csv";
    std::fstream fin_edges; 
    fin_edges.open(filename_edges, std::ios::in);

    header_line = true;
    int count_edges = 0;
    while (getline(fin_edges, line)) {
        
        count_edges++;
  
        std::stringstream s(line);
        std::map<std::string, std::string> attributes;

        int position = 0;

        std::string from,to;
        std::string id;
        while (getline(s, word, ',')) {
            if (header_line) {
                header_edge.push_back(word);
                column_count = position;
            } else {
                if (position==3) {
                    id = word;
                } else if (position==1) {
                    from = "ID_" + word;
                } else if (position==2) {
                    to = "ID_" + word;
                } else {
                    if (word.compare("") != 0 && word.compare(" ") != 0 && word.compare("\r") != 0 && word.compare("\n") != 0) {
                        attributes[header_edge[position]] = word;
                    }
                }
            }
            position++;
        }
        if (header_line) {
            header_line = false;
        } else {
            G.add_edge(from, id, to, attributes);
        }
    }
    fin_edges.close();
}

void GNF1(graph& G) {
    graph::vmap old_graph = G.work;
    for (const auto& node : old_graph) {
        std::string node_id = node.first;
        for (const auto& mult_att : node.second->multi_attributes) {
            for (const auto& extract_att : mult_att.second) {
                G.add_edge_GNF1(node_id, mult_att.first, extract_att);
            }
        }
        vertex::att_map new_attributes;
        for (const auto& att : node.second->attributes) {
            if (G.work.find(att.second) != G.work.end()) {
                G.add_edge_GNF1(node_id, att.first, att.second);
            } else {
                new_attributes[att.first] = att.second;
            }
        }
        vertex::multi_att_map multi_attributes;
        G.work[node_id]->multi_attributes = multi_attributes;
        G.work[node_id]->attributes = new_attributes;
    }
}

void GNF2_dblp(graph& G) {
    for (auto& node : G.work) {
        std::string node_id = node.first;
        if ((node.second->attributes.find("~label") != node.second->attributes.end()) && (node.second->attributes["~label"].compare(":https://dblp.org/rdf/schema#Publication") == 0)) {
            if ((node.second->attributes.find("https://dblp.org/rdf/schema#isbn") != node.second->attributes.end() ) && (node.second->attributes.find("https://dblp.org/rdf/schema#doi") != node.second->attributes.end() ) && (node.second->attributes.find("https://dblp.org/rdf/schema#listedOnTocPage") != node.second->attributes.end() )) {
                bool first_att = true;
                for (auto& neig_pair : node.second->labeled_adj) {
                    if (neig_pair.second.compare("https://dblp.org/rdf/schema#authoredBy") == 0 && neig_pair.first->attributes["~label"].compare(":https://dblp.org/rdf/schema#Group") == 0) {
                        if (neig_pair.first->attributes.find("https://dblp.org/rdf/schema#primaryAffiliation") != neig_pair.first->attributes.end()) {
                            if (first_att){
                                first_att = false;
                                node.second->add_attribute("https://dblp.org/rdf/schema#primaryAffiliation", neig_pair.first->attributes["https://dblp.org/rdf/schema#primaryAffiliation"]);
                            }
                            GLOBAL_ATTRIBUTE_COUNT--;
                            std::cout << "did this" << std::endl;
                            neig_pair.first->attributes.erase("https://dblp.org/rdf/schema#primaryAffiliation");
                        }
                    }
                }
            }
        }

        if ((node.second->attributes.find("~label") != node.second->attributes.end()) && (node.second->attributes["~label"].compare(":https://dblp.org/rdf/schema#Publication") == 0)) {
            if ((node.second->attributes.find("https://dblp.org/rdf/schema#publishedAsPartOf") != node.second->attributes.end() ) && (node.second->attributes.find("https://dblp.org/rdf/schema#publishedInJournal") != node.second->attributes.end() ) && (node.second->attributes.find("https://dblp.org/rdf/schema#documentPage") != node.second->attributes.end() )) {
                bool first_att = true;
                for (auto& neig_pair : node.second->labeled_adj) {
                    if (neig_pair.second.compare("https://dblp.org/rdf/schema#authoredBy") == 0 && neig_pair.first->attributes["~label"].compare(":https://dblp.org/rdf/schema#Group") == 0) {
                        if (neig_pair.first->attributes.find("https://dblp.org/rdf/schema#webpage") != neig_pair.first->attributes.end()) {
                            if (first_att){
                                first_att = false;
                                node.second->add_attribute("https://dblp.org/rdf/schema#webpage", neig_pair.first->attributes["https://dblp.org/rdf/schema#webpage"]);
                            }
                            GLOBAL_ATTRIBUTE_COUNT--;
                            std::cout << "did that" << std::endl;
                            neig_pair.first->attributes.erase("https://dblp.org/rdf/schema#webpage");
                        }
                    }
                }
            }
        }

        if (node.second->attributes.find("https://dblp.org/rdf/schema#bibtexType") != node.second->attributes.end() ) {
            std::string tmp = node.second->attributes["https://dblp.org/rdf/schema#bibtexType"];
            if (tmp.compare("http://purl.org/net/nknouf/ns/bibtex#Article")==0) { //|| tmp.compare("http://purl.org/net/nknouf/ns/bibtex#Misc")==0 || tmp.compare("http://purl.org/net/nknouf/ns/bibtex#Book")==0) {
                if (node.second->attributes.find("https://dblp.org/rdf/schema#publishedInJournalVolume") != node.second->attributes.end()) {
                    for (auto& neig_pair : node.second->labeled_adj) {
                        if (neig_pair.second.compare("https://dblp.org/rdf/schema#publishedAsPartOf") == 0) {
                            if ((neig_pair.first->attributes.find("https://dblp.org/rdf/schema#publishedInJournalVolume") != neig_pair.first->attributes.end()) && (neig_pair.first->attributes["https://dblp.org/rdf/schema#publishedInJournalVolume"].compare("") != 0) ) {
                                if (node.second->attributes["https://dblp.org/rdf/schema#publishedInJournalVolume"].compare(neig_pair.first->attributes["https://dblp.org/rdf/schema#publishedInJournalVolume"]) != 0) {
                                    std::cout << "pub" << std::endl;
                                    std::cout << node.first << std::endl;
                                    std::cout << node.second->attributes["https://dblp.org/rdf/schema#publishedInJournalVolume"] << " : " << neig_pair.first->attributes["https://dblp.org/rdf/schema#publishedInJournalVolume"] << std::endl;
                                }
                                assert(node.second->attributes["https://dblp.org/rdf/schema#publishedInJournalVolume"].compare(neig_pair.first->attributes["https://dblp.org/rdf/schema#publishedInJournalVolume"]) == 0);
                            } else {
                                neig_pair.first->add_attribute("https://dblp.org/rdf/schema#publishedInJournalVolume", node.second->attributes["https://dblp.org/rdf/schema#publishedInJournalVolume"]);
                            }
                            node.second->attributes.erase("https://dblp.org/rdf/schema#publishedInJournalVolume");
                            GLOBAL_ATTRIBUTE_COUNT--;
                            break;
                        }
                    }
                }
            }
            if (tmp.compare("http://purl.org/net/nknouf/ns/bibtex#Inproceedings")==0) { //|| tmp.compare("http://purl.org/net/nknouf/ns/bibtex#Misc")==0 || tmp.compare("http://purl.org/net/nknouf/ns/bibtex#Book")==0) {
                if (node.second->attributes.find("https://dblp.org/rdf/schema#publishedInSeriesVolume") != node.second->attributes.end()) {
                    for (auto& neig_pair : node.second->labeled_adj) {
                        if (neig_pair.second.compare("https://dblp.org/rdf/schema#publishedAsPartOf") == 0) {
                            if ((neig_pair.first->attributes.find("https://dblp.org/rdf/schema#publishedInSeriesVolume") != neig_pair.first->attributes.end()) && (neig_pair.first->attributes["https://dblp.org/rdf/schema#publishedInSeriesVolume"].compare("") != 0) ) {
                                if (node.second->attributes["https://dblp.org/rdf/schema#publishedInSeriesVolume"].compare(neig_pair.first->attributes["https://dblp.org/rdf/schema#publishedInSeriesVolume"]) != 0) {
                                    std::cout << "pub" << std::endl;
                                    std::cout << node.first << std::endl;
                                    std::cout << node.second->attributes["https://dblp.org/rdf/schema#publishedInSeriesVolume"] << " : " << neig_pair.first->attributes["https://dblp.org/rdf/schema#publishedInSeriesVolume"] << std::endl;
                                }
                                assert(node.second->attributes["https://dblp.org/rdf/schema#publishedInSeriesVolume"].compare(neig_pair.first->attributes["https://dblp.org/rdf/schema#publishedInSeriesVolume"]) == 0);
                            } else {
                                neig_pair.first->add_attribute("https://dblp.org/rdf/schema#publishedInSeriesVolume", node.second->attributes["https://dblp.org/rdf/schema#publishedInSeriesVolume"]);
                            }
                            node.second->attributes.erase("https://dblp.org/rdf/schema#publishedInSeriesVolume");
                            GLOBAL_ATTRIBUTE_COUNT--;
                            if (neig_pair.first->attributes.find("https://dblp.org/rdf/schema#yearOfPublication") != neig_pair.first->attributes.end()) {
                                if ((node.second->attributes.find("https://dblp.org/rdf/schema#yearOfPublication") != node.second->attributes.end()) && (node.second->attributes["https://dblp.org/rdf/schema#yearOfPublication"].compare("")!=0) ) {
                                    if (node.second->attributes["https://dblp.org/rdf/schema#yearOfPublication"].compare(neig_pair.first->attributes["https://dblp.org/rdf/schema#yearOfPublication"]) != 0) {
                                        std::cout << "pub" << std::endl;
                                        std::cout << node.first << std::endl;
                                        std::cout << node.second->attributes["https://dblp.org/rdf/schema#yearOfPublication"] << " : " << neig_pair.first->attributes["https://dblp.org/rdf/schema#yearOfPublication"] << std::endl;
                                    }
                                    //assert(node.second->attributes["https://dblp.org/rdf/schema#yearOfPublication"].compare(neig_pair.first->attributes["https://dblp.org/rdf/schema#yearOfPublication"]) == 0);
                                } else {
                                    node.second->add_attribute("https://dblp.org/rdf/schema#yearOfPublication", neig_pair.first->attributes["https://dblp.org/rdf/schema#yearOfPublication"]);
                                }
                                neig_pair.first->attributes.erase("https://dblp.org/rdf/schema#yearOfPublication");
                                GLOBAL_ATTRIBUTE_COUNT--;
                            }
                            break;
                        }
                    }
                }
            }
        }
    }
}

void GNF3_dblp(graph& G) {
    for (auto& node : G.work) {
        std::string node_id = node.first;
        // if ((node.second->attributes.find("https://dblp.org/rdf/schema#publishedInJournal") != node.second->attributes.end()) && (node.second->attributes.find("https://dblp.org/rdf/schema#publishedIn") != node.second->attributes.end()) ) {
        //     std::string resource = node.second->attributes["https://dblp.org/rdf/schema#publishedInJournal"];
        //     graph::vmap::iterator itr = G.work.find(resource);
        //     if (itr == G.work.end()) {
        //         G.add_vertex(resource,resource);
        //         G.vertex_add_attribute(resource, "~label", "Journal label");
        //     }
        //     node.second->attributes.erase("https://dblp.org/rdf/schema#publishedInJournal");
        //     GLOBAL_ATTRIBUTE_COUNT--;
        //     G.vertex_add_attribute(resource, "https://dblp.org/rdf/schema#publishedIn", node.second->attributes["https://dblp.org/rdf/schema#publishedIn"]);
        //     node.second->attributes.erase("https://dblp.org/rdf/schema#publishedIn");
        //     G.add_edge_GNF1(node.first, "https://dblp.org/rdf/schema#publishedInJournal", resource);

        // } else
        if ((node.second->attributes.find("https://dblp.org/rdf/schema#publishedInSeries") != node.second->attributes.end()) && (node.second->attributes.find("https://dblp.org/rdf/schema#publishedIn") != node.second->attributes.end()) ) {
            std::string resource = node.second->attributes["https://dblp.org/rdf/schema#publishedInSeries"];
            graph::vmap::iterator itr = G.work.find(resource);
            if (itr == G.work.end()) {
                G.add_vertex(resource,resource);
                G.vertex_add_attribute(resource, "~label", "Series label");
            }
            node.second->attributes.erase("https://dblp.org/rdf/schema#publishedInSeries");
            G.vertex_add_attribute(resource, "https://dblp.org/rdf/schema#publishedIn", node.second->attributes["https://dblp.org/rdf/schema#publishedIn"]);
            node.second->attributes.erase("https://dblp.org/rdf/schema#publishedIn");
            GLOBAL_ATTRIBUTE_COUNT--;
            G.add_edge_GNF1(node.first, "https://dblp.org/rdf/schema#publishedInSeries", resource);
        }
        if ((node.second->attributes.find("https://dblp.org/rdf/schema#publishersAddress") != node.second->attributes.end()) && (node.second->attributes.find("https://dblp.org/rdf/schema#publishedBy") != node.second->attributes.end()) ) {
            std::string resource = node.second->attributes["https://dblp.org/rdf/schema#publishedBy"];
            graph::vmap::iterator itr = G.work.find(resource);
            if (itr == G.work.end()) {
                G.add_vertex(resource,resource);
                G.vertex_add_attribute(resource, "~label", "Publisher label");
            }
            node.second->attributes.erase("https://dblp.org/rdf/schema#publishedBy");
            G.vertex_add_attribute(resource, "https://dblp.org/rdf/schema#publishersAddress", node.second->attributes["https://dblp.org/rdf/schema#publishersAddress"]);
            node.second->attributes.erase("https://dblp.org/rdf/schema#publishersAddress");
            GLOBAL_ATTRIBUTE_COUNT--;
            G.add_edge_GNF1(node.first, "https://dblp.org/rdf/schema#publishedBy", resource);
        }
        if ((node.second->attributes["~label"].compare(":https://dblp.org/rdf/schema#Person") == 0) && (node.second->attributes.find("https://dblp.org/rdf/schema#primaryHomepage") != node.second->attributes.end()) && (node.second->attributes.find("https://dblp.org/rdf/schema#creatorName") != node.second->attributes.end()) ) {
            std::string resource = node.second->attributes["https://dblp.org/rdf/schema#primaryHomepage"] + " " + node.second->attributes["https://dblp.org/rdf/schema#creatorName"];
            graph::vmap::iterator itr = G.work.find(resource);
            if (itr == G.work.end()) {
                G.add_vertex(resource,resource);
                G.vertex_add_attribute(resource, "~label", "homepage");
                G.vertex_add_attribute(resource, "https://dblp.org/rdf/schema#primaryHomepage", node.second->attributes["https://dblp.org/rdf/schema#primaryHomepage"]);
                G.vertex_add_attribute(resource, "https://dblp.org/rdf/schema#creatorName", node.second->attributes["https://dblp.org/rdf/schema#creatorName"]);
            }
            node.second->attributes.erase("https://dblp.org/rdf/schema#creatorName");
            node.second->attributes.erase("https://dblp.org/rdf/schema#primaryHomepage");
            GLOBAL_ATTRIBUTE_COUNT--;
            G.vertex_add_attribute(resource, "https://dblp.org/rdf/schema#homepage", node.second->attributes["https://dblp.org/rdf/schema#homepage"]);
            node.second->attributes.erase("https://dblp.org/rdf/schema#homepage");
            GLOBAL_ATTRIBUTE_COUNT--;
            G.add_edge_GNF1(node.first, "homepage", resource);
        }
        std::string fds[] = { "https://dblp.org/rdf/schema#publishedInJournal", "https://dblp.org/rdf/schema#publishedAsPartOf", "https://dblp.org/rdf/schema#publishedInBook", "https://dblp.org/rdf/schema#yearOfEvent"};
        if ((node.second->attributes["~label"].compare(":https://dblp.org/rdf/schema#Publication") == 0) && (node.second->attributes.find("https://dblp.org/rdf/schema#listedOnTocPage") != node.second->attributes.end())){
            std::string resource = node.second->attributes["https://dblp.org/rdf/schema#listedOnTocPage"];
            graph::vmap::iterator itr = G.work.find(resource);
            if (itr == G.work.end()) {
                G.add_vertex(resource,resource);
                for (const auto& fd : fds) {
                    if (node.second->attributes.find(fd) != node.second->attributes.end()) {
                        G.vertex_add_attribute(resource, fd, node.second->attributes[fd]);
                    }
                }
                G.vertex_add_attribute(resource, "~label", "TocPage");
            }
            node.second->attributes.erase("https://dblp.org/rdf/schema#listedOnTocPage");
            for (const auto& fd : fds) {
                if (node.second->attributes.find(fd) != node.second->attributes.end()) {
                    node.second->attributes.erase(fd);
                    GLOBAL_ATTRIBUTE_COUNT--;
                }
            }
            G.add_edge_GNF1(node.first, "https://dblp.org/rdf/schema#listedOnTocPage", resource);
        }
        std::string fds2[] = { "https://dblp.org/rdf/schema#publishedInBookChapter", "https://dblp.org/rdf/schema#monthOfPublication", "https://dblp.org/rdf/schema#primaryDocumentPage", "https://dblp.org/rdf/schema#pagination", "https://dblp.org/rdf/schema#documentPage" };
        if ((node.second->attributes["~label"].compare(":https://dblp.org/rdf/schema#Publication") == 0) && (node.second->attributes.find("https://dblp.org/rdf/schema#documentPage") != node.second->attributes.end())  && (node.second->attributes.find("https://dblp.org/rdf/schema#pagination") != node.second->attributes.end())){
            std::string resource = node.second->attributes["https://dblp.org/rdf/schema#pagination"] + "_" + node.second->attributes["https://dblp.org/rdf/schema#documentPage"];
            graph::vmap::iterator itr = G.work.find(resource);
            if (itr == G.work.end()) {
                G.add_vertex(resource,resource);
                for (const auto& fd : fds2) {
                    if (node.second->attributes.find(fd) != node.second->attributes.end()) {
                        G.vertex_add_attribute(resource, fd, node.second->attributes[fd]);
                    }
                }
                G.vertex_add_attribute(resource, "~label", "pages");
            }
            for (const auto& fd : fds2) {
                if (node.second->attributes.find(fd) != node.second->attributes.end()) {
                    node.second->attributes.erase(fd);
                    GLOBAL_ATTRIBUTE_COUNT--;
                }
            }
            GLOBAL_ATTRIBUTE_COUNT++;
            G.add_edge_GNF1(node.first, "pages", resource);
        }
    }
}

void GNF2_mimic(graph& G) {

}

void GNF3_mimic(graph& G) {
    for (auto& node : G.work) {
        vertex::att_map::iterator n_itr = node.second->attributes.find("Discharge_location");
        if (node.second->attributes.end() != n_itr) {
            std::string resource = node.second->attributes["Discharge_location"];
            node.second->attributes.erase("Discharge_location");
            GLOBAL_ATTRIBUTE_COUNT--;
            graph::vmap::iterator itr = G.work.find(resource);
            bool new_node = false;
            if (itr == G.work.end()) {
                G.add_vertex(resource,resource);
                G.vertex_add_attribute(resource, "~label", "Discharge_location");
                new_node = true;
            }
            G.add_edge_GNF1(node.first,"Discharge_location",resource);

            n_itr = node.second->attributes.find("Hospital_expire_flag");
            if (node.second->attributes.end() != n_itr) {
                if (new_node) {
                    G.vertex_add_attribute(resource,"Hospital_expire_flag",node.second->attributes["Hospital_expire_flag"]);
                }
                node.second->attributes.erase("Hospital_expire_flag");
                GLOBAL_ATTRIBUTE_COUNT--;
            }
        }
        n_itr = node.second->attributes.find("DOD");
        if (node.second->attributes.end() != n_itr) {
            std::string resource = node.second->attributes["DOD"];
            node.second->attributes.erase("DOD");
            GLOBAL_ATTRIBUTE_COUNT--;
            graph::vmap::iterator itr = G.work.find(resource);
            bool new_node = false;
            if (itr == G.work.end()) {
                G.add_vertex(resource,resource);
                G.vertex_add_attribute(resource, "~label", "DOD");
                new_node = true;
            }
            G.add_edge_GNF1(node.first,"DOD",resource);

            n_itr = node.second->attributes.find("Expire_flag");
            if (node.second->attributes.end() != n_itr) {
                if (new_node) {
                    G.vertex_add_attribute(resource,"Expire_flag",node.second->attributes["Expire_flag"]);
                }
                node.second->attributes.erase("Expire_flag");
                GLOBAL_ATTRIBUTE_COUNT--;
            }
        }
    }
}

void GNF2_go(graph& G) {

}

void GNF3_go(graph& G) {
    for (auto& node : G.work) {
        vertex::att_map::iterator n_itr = node.second->attributes.find("http://www.geneontology.org/formats/oboInOwl#creation_date");
        if (node.second->attributes.end() != n_itr) {
            std::string resource = node.second->attributes["http://www.geneontology.org/formats/oboInOwl#creation_date"];
            node.second->attributes.erase("http://www.geneontology.org/formats/oboInOwl#creation_date");
            GLOBAL_ATTRIBUTE_COUNT--;
            graph::vmap::iterator itr = G.work.find(resource);
            bool new_node = false;
            if (itr == G.work.end()) {
                G.add_vertex(resource,resource);
                G.vertex_add_attribute(resource, "~label", "http://www.geneontology.org/formats/oboInOwl#creation_date");
                new_node = true;
            }
            G.add_edge_GNF1(node.first,"http://www.geneontology.org/formats/oboInOwl#creation_date",resource);

            n_itr = node.second->attributes.find("http://www.geneontology.org/formats/oboInOwl#created_by");
            if (node.second->attributes.end() != n_itr) {
                if (new_node) {
                    G.vertex_add_attribute(resource,"http://www.geneontology.org/formats/oboInOwl#created_by",node.second->attributes["http://www.geneontology.org/formats/oboInOwl#created_by"]);
                }
                node.second->attributes.erase("http://www.geneontology.org/formats/oboInOwl#created_by");
                GLOBAL_ATTRIBUTE_COUNT--;
            }
        }
    }
}

void GNF2_offshore(graph& G) {
    for (auto& node : G.work) {
        if ((node.second->attributes.find("\"~label\"") != node.second->attributes.end()) && (node.second->attributes["\"~label\""].compare("\":Intermediary\"") == 0)) {
            // std::cout << "entered rule" << std::endl;
            for (auto& neigh : node.second->labeled_adj) {
                if ((neigh.second.compare("\"intermediary_of\"") == 0) && (neigh.first->attributes.find("\"sourceID\"") != neigh.first->attributes.end()) ) {
                    // std::cout << "doing shit" << std::endl;
                    neigh.first->attributes.erase("\"sourceID\"");
                    GLOBAL_ATTRIBUTE_COUNT--;
                }
            }
        }
        // if ((node.second->attributes.find("label") != node.second->attributes.end())) {
        //     std::cout << "non brackets" << std::endl;
        //     std::cout << node.second->attributes["label"] << std::endl;
        //     exit(1);
        // }
        // if ((node.second->attributes.find("\"label\"") != node.second->attributes.end())) {
        //     std::cout << "brackets" << std::endl;
        //     std::cout << node.second->attributes["\"label\""] << std::endl;
        //     exit(1);
        // }
    }
}

void GNF3_offshore(graph& G) {
    for (auto& node : G.work) {
        vertex::att_map::iterator n_itr = node.second->attributes.find("\"service_provider\"");
        if (node.second->attributes.end() != n_itr) {
            std::string resource = node.second->attributes["\"service_provider\""];
            node.second->attributes.erase("\"service_provider\"");
            graph::vmap::iterator itr = G.work.find(resource);
            bool new_node = false;
            if (itr == G.work.end()) {
                G.add_vertex(resource,resource);
                G.vertex_add_attribute(resource, "\"~label\"", "\"service_provider\"");
                new_node = true;
            }
            G.add_edge_GNF1(node.first,"\"service_provider\"",resource);

            n_itr = node.second->attributes.find("\"sourceID\"");
            if (node.second->attributes.end() != n_itr) {
                if (new_node) {
                    G.vertex_add_attribute(resource,"\"sourceID\"",node.second->attributes["\"sourceID\""]);
                }
                node.second->attributes.erase("\"sourceID\"");
                GLOBAL_ATTRIBUTE_COUNT--;
            }
            n_itr = node.second->attributes.find("\"valid_until\"");
            if (node.second->attributes.end() != n_itr) {
                if (new_node) {
                    G.vertex_add_attribute(resource,"\"valid_until\"",node.second->attributes["\"valid_until\""]);
                }
                node.second->attributes.erase("\"valid_until\"");
                GLOBAL_ATTRIBUTE_COUNT--;
            }
        }
        vertex::att_map::iterator n_itr1 = node.second->attributes.find("\"ibcRUC\"");
        vertex::att_map::iterator n_itr2 = node.second->attributes.find("\"struck_off_date\"");
        vertex::att_map::iterator n_itr3 = node.second->attributes.find("\"lastEditTimestamp\"");
        if (node.second->attributes.end() != n_itr1 && node.second->attributes.end() != n_itr2 && node.second->attributes.end() != n_itr3) {
            std::string str1 = node.second->attributes["\"ibcRUC\""].substr(0, node.second->attributes["\"ibcRUC\""].length()-2);
            std::string str2 = node.second->attributes["\"struck_off_date\""].substr(1, node.second->attributes["\"struck_off_date\""].length()-2);
            std::string str3 = node.second->attributes["\"lastEditTimestamp\""].substr(1);
            std::string resource = str1 + "_" + str2 + "_" + str3;
            graph::vmap::iterator itr = G.work.find(resource);
            bool new_node = false;
            if (itr == G.work.end()) {
                G.add_vertex(resource,resource);
                G.vertex_add_attribute(resource, "\"~label\"", "\"InactDate\"");
                G.vertex_add_attribute(resource,"\"ibcRUC\"",node.second->attributes["\"ibcRUC\""]);
                G.vertex_add_attribute(resource,"\"struck_off_date\"",node.second->attributes["\"struck_off_date\""]);
                G.vertex_add_attribute(resource,"\"lastEditTimestamp\"",node.second->attributes["\"lastEditTimestamp\""]);
                new_node = true;
            }
            node.second->attributes.erase("\"ibcRUC\"");
            node.second->attributes.erase("\"struck_off_date\"");
            node.second->attributes.erase("\"lastEditTimestamp\"");
            GLOBAL_ATTRIBUTE_COUNT--;
            GLOBAL_ATTRIBUTE_COUNT--;
            G.add_edge_GNF1(node.first,"\"InactDate\"",resource);

            n_itr = node.second->attributes.find("\"inactivation_date\"");
            if (node.second->attributes.end() != n_itr) {
                if (new_node) {
                    G.vertex_add_attribute(resource,"\"inactivation_date\"",node.second->attributes["\"inactivation_date\""]);
                }
                node.second->attributes.erase("\"inactivation_date\"");
                GLOBAL_ATTRIBUTE_COUNT--;
            }
        }
        vertex::att_map::iterator n_itr4 = node.second->attributes.find("\"closed_date\"");
        vertex::att_map::iterator n_itr5 = node.second->attributes.find("\"incorporation_date\"");
        if (node.second->attributes.end() != n_itr4 && node.second->attributes.end() != n_itr5) {
            std::string str4 = node.second->attributes["\"closed_date\""].substr(0, node.second->attributes["\"closed_date\""].length()-2);
            std::string str5 = node.second->attributes["\"incorporation_date\""].substr(1);
            std::string resource = str4 + "_" + str5;
            graph::vmap::iterator itr = G.work.find(resource);
            bool new_node = false;
            if (itr == G.work.end()) {
                G.add_vertex(resource,resource);
                G.vertex_add_attribute(resource, "\"~label\"", "\"RegDate\"");
                G.vertex_add_attribute(resource,"\"closed_date\"",node.second->attributes["\"closed_date\""]);
                G.vertex_add_attribute(resource,"\"incorporation_date\"",node.second->attributes["\"incorporation_date\""]);
                new_node = true;
            }
            node.second->attributes.erase("\"closed_date\"");
            node.second->attributes.erase("\"incorporation_date\"");
            GLOBAL_ATTRIBUTE_COUNT--;
            GLOBAL_ATTRIBUTE_COUNT--;
            G.add_edge_GNF1(node.first,"\"RegDate\"",resource);

            n_itr = node.second->attributes.find("\"registration_date\"");
            if (node.second->attributes.end() != n_itr) {
                if (new_node) {
                    G.vertex_add_attribute(resource,"\"registration_date\"",node.second->attributes["\"registration_date\""]);
                }
                node.second->attributes.erase("\"registration_date\"");
                GLOBAL_ATTRIBUTE_COUNT--;
            }
        }
        vertex::att_map::iterator n_itr6 = node.second->attributes.find("\"country_codes\"");
        vertex::att_map::iterator n_itr7 = node.second->attributes.find("\"company_type\"");
        if (node.second->attributes.end() != n_itr6 && node.second->attributes.end() != n_itr7) {
            std::string resource = node.second->attributes["\"country_codes\""];
            graph::vmap::iterator itr = G.work.find(resource);
            bool new_node = false;
            if (itr == G.work.end()) {
                G.add_vertex(resource,resource);
                G.vertex_add_attribute(resource, "\"~label\"", "\"country_codes\"");
                new_node = true;
            }
            node.second->attributes.erase("\"country_codes\"");
            G.add_edge_GNF1(node.first,"\"country_codes\"",resource);

            n_itr = node.second->attributes.find("\"company_type\"");
            if (node.second->attributes.end() != n_itr) {
                if (new_node) {
                    G.vertex_add_attribute(resource,"\"company_type\"",node.second->attributes["\"company_type\""]);
                }
                node.second->attributes.erase("\"company_type\"");
                GLOBAL_ATTRIBUTE_COUNT--;
            }
        }
    }  
}

void GNF2_northwind(graph& G) {
    std::string fds[] = { "shipAddress", "shipCity", "shipCountry", "shipName", "shipPostalCode", "shipRegion", "customerID"};
    for (auto& node : G.work) {
        if ((node.second->attributes.find("~label") != node.second->attributes.end()) && (node.second->attributes["~label"].compare("Order") == 0)) {
            for (auto& neig_pair : node.second->labeled_rev_adj) {
                if (neig_pair.second.compare("PURCHASED") == 0) {
                    for (const auto& pro : fds) {
                        if (node.second->attributes.find(pro) != node.second->attributes.end()) {
                            if (neig_pair.first->attributes.find(pro) != neig_pair.first->attributes.end()) {
                                if (neig_pair.first->attributes[pro].compare(node.second->attributes[pro])) {
                                    std::cout << node.first << std::endl;
                                    std::cout << pro << std::endl;
                                    std::cout << neig_pair.first->attributes[pro] << " : " << node.second->attributes[pro] << std::endl;
                                    std::cout << std::endl;
                                }
                                assert(neig_pair.first->attributes[pro].compare(node.second->attributes[pro]) == 0 );
                            } else {
                                neig_pair.first->add_attribute(pro,node.second->attributes[pro]);
                            }
                            node.second->attributes.erase(pro);
                            GLOBAL_ATTRIBUTE_COUNT--;
                        }
                    }
                }
            }
        }
    }
}

void GNF3_northwind(graph& G) {
    std::string fds[] = { "shipCity", "shipCountry", "country","shipPostalCode"};
    std::string fds2[] = { "unitsInStock", "unitPrice", "quantityPerUnit", "unitsOnOrder", "reorderLevel", "discontinued" };
    for (auto& node : G.work) {
        if (node.second->attributes.find("~label") != node.second->attributes.end()) {
            if (node.second->attributes["~label"].compare("Customer") == 0 || node.second->attributes["~label"].compare("Supplier") == 0) {
                std::string resource = node.second->attributes["city"];
                graph::vmap::iterator itr = G.work.find(resource);
                if (itr == G.work.end()) {
                    G.add_vertex(resource,resource);
                    for (const auto& fd : fds) {
                        if (node.second->attributes.find(fd) != node.second->attributes.end()) {
                            G.vertex_add_attribute(resource, fd, node.second->attributes[fd]);
                        }
                    }
                    G.vertex_add_attribute(resource, "~label", "City");
                }
                node.second->attributes.erase("city");
                for (const auto& fd : fds) {
                    if (node.second->attributes.find(fd) != node.second->attributes.end()) {
                        node.second->attributes.erase(fd);
                        GLOBAL_ATTRIBUTE_COUNT--;
                    }
                }
                G.add_edge_GNF1(node.first, "city", resource);
            } else if (node.second->attributes["~label"].compare("Product") == 0) {
                std::string resource = node.second->attributes["unitPrice"] + "_" + node.second->attributes["unitsInStock"];
                graph::vmap::iterator itr = G.work.find(resource);
                if (itr == G.work.end()) {
                    G.add_vertex(resource,resource);
                    for (const auto& fd : fds2) {
                        if (node.second->attributes.find(fd) != node.second->attributes.end()) {
                            G.vertex_add_attribute(resource, fd, node.second->attributes[fd]);
                        }
                    }
                    G.vertex_add_attribute(resource, "~label", "ProductDetails");
                }
                for (const auto& fd : fds2) {
                    if (node.second->attributes.find(fd) != node.second->attributes.end()) {
                        node.second->attributes.erase(fd);
                        GLOBAL_ATTRIBUTE_COUNT--;
                    }
                }
                G.add_edge_GNF1(node.first, "ProductDetails", resource);
            // } else if (node.second->attributes["~label"].compare("Supplier") == 0) {
            //     std::string resource = node.second->attributes["city"];
            //     graph::vmap::iterator itr = G.work.find(resource);
            //     if (itr == G.work.end()) {
            //         G.add_vertex(resource,resource);
            //         G.vertex_add_attribute(resource, "country", node.second->attributes["country"]);
            //         G.vertex_add_attribute(resource, "~label", "City");
            //     }
            //     node.second->attributes.erase("city");
            //     GLOBAL_ATTRIBUTE_COUNT--;
            //     node.second->attributes.erase("country");
            //     G.add_edge_GNF1(node.first, "city", resource);
            }
        }
    }
}

// MATCH (n:NODE)-[r:`http://dbpedia.org/property/spacecraftType`]->(m:NODE)
// WHERE m.`http://dbpedia.org/property/applications` IS NOT NULL AND n.`http://dbpedia.org/property/missionType` IS NOT NULL
// WITH m.`http://dbpedia.org/property/applications` AS pub, COLLECT(DISTINCT n.`http://dbpedia.org/property/missionType`) AS b
// //WHERE SIZE(b) > 1
// RETURN pub, b LIMIT 25

// MATCH (n:NODE)-[r:`http://dbpedia.org/property/president`]->(m:NODE)
// WHERE n.`http://dbpedia.org/property/finalsSeries` IS NOT NULL
// WITH m AS pub, COLLECT(DISTINCT n.`http://dbpedia.org/property/finalsSeries`) AS b
// //WHERE SIZE(b) > 1
// RETURN pub.IRI, b

// MATCH (n:NODE)-[r:`http://dbpedia.org/ontology/kingdom`]->(m:NODE)
// WHERE m.`http://dbpedia.org/property/taxon` IS NOT NULL AND n.`http://dbpedia.org/property/regnum` IS NOT NULL
// WITH m.`http://dbpedia.org/property/taxon` AS pub, COLLECT(DISTINCT n.`http://dbpedia.org/property/regnum`) AS b
// //WHERE SIZE(b) > 1
// RETURN pub, b

void GNF2_dbpedia(graph& G) {
    for (auto& node : G.work) {
        auto& point = node.second->attributes;
        // if (point.find("http://dbpedia.org/property/applications") != point.end()) {
        //     for (auto& neigh : node.second->labeled_rev_adj) {
        //         if (neigh.second.compare("http://dbpedia.org/property/spacecraftType") == 0) {
        //             if (neigh.first->attributes.find("http://dbpedia.org/property/missionType") != neigh.first->attributes.end()) {
        //                 if (point.find("http://dbpedia.org/property/missionType") != point.end()) {
        //                     node.second->add_attribute("http://dbpedia.org/property/missionType",neigh.first->attributes["http://dbpedia.org/property/missionType"]);
        //                 }
        //                 neigh.first->attributes.erase("http://dbpedia.org/property/missionType");
        //                 GLOBAL_ATTRIBUTE_COUNT--;
        //             }
        //         }
        //     }
        // }
        if (point.find("http://dbpedia.org/property/taxon") != point.end()) {
            for (auto& neigh : node.second->labeled_rev_adj) {
                if (neigh.second.compare("http://dbpedia.org/ontology/kingdom") == 0) {
                    if (neigh.first->attributes.find("http://dbpedia.org/property/regnum") != neigh.first->attributes.end()) {
                        if (point.find("http://dbpedia.org/property/regnum") != point.end()) {
                            node.second->add_attribute("http://dbpedia.org/property/regnum",neigh.first->attributes["http://dbpedia.org/property/regnum"]);
                        }
                        neigh.first->attributes.erase("http://dbpedia.org/property/regnum");
                        GLOBAL_ATTRIBUTE_COUNT--;
                    }
                }
            }
        }
        if (point.find("http://dbpedia.org/property/finalsSeries") != point.end()) {
            for (auto& neigh : node.second->labeled_adj) {
                if (neigh.second.compare("http://dbpedia.org/property/president") == 0) {
                    if (neigh.first->attributes.find("http://dbpedia.org/property/finalsSeries") != neigh.first->attributes.end()) {
                        neigh.first->add_attribute("http://dbpedia.org/property/finalsSeries",neigh.first->attributes["http://dbpedia.org/property/finalsSeries"]);
                    }
                    point.erase("http://dbpedia.org/property/finalsSeries");
                    GLOBAL_ATTRIBUTE_COUNT--;
                }
            }
        }
    }
}

// MATCH (n)
// WHERE n.`http://dbpedia.org/property/applications` IS NOT NULL
// RETURN COUNT(n) AS nodesWithAge, COUNT(DISTINCT n.`http://dbpedia.org/property/applications`) AS distinctAges

// MATCH (n:NODE)
// WHERE n.`http://dbpedia.org/property/pushpinMap` IS NOT NULL AND n.`http://dbpedia.org/property/translitLang` IS NOT NULL
// WITH n.`http://dbpedia.org/property/pushpinMap` AS pub, COLLECT(DISTINCT n.`http://dbpedia.org/property/translitLang`) AS b
// //WHERE SIZE(b) > 1
// RETURN pub, b

void GNF3_dbpedia(graph& G) {
    for (auto& node : G.work) {
        auto& point = node.second->attributes;
        // if ((point.find("http://dbpedia.org/property/applications") != point.end()) && (point.find("http://dbpedia.org/property/missionType") != point.end())) {
        //     std::string resource = node.second->attributes["http://dbpedia.org/property/applications"];
        //     graph::vmap::iterator itr = G.work.find(resource);
        //     if (itr == G.work.end()) {
        //         G.add_vertex(resource,resource);
        //         G.vertex_add_attribute(resource, "http://dbpedia.org/property/missionType", node.second->attributes["http://dbpedia.org/property/missionType"]);
        //     }
        //     node.second->attributes.erase("http://dbpedia.org/property/applications");
        //     node.second->attributes.erase("http://dbpedia.org/property/missionType");
        //     GLOBAL_ATTRIBUTE_COUNT--;
        //     G.add_edge_GNF1(node.first, "http://dbpedia.org/property/applications", resource);
        // }
        if ((point.find("http://dbpedia.org/property/pushpinMap") != point.end()) && (point.find("http://dbpedia.org/property/translitLang") != point.end())) {
            std::string resource = node.second->attributes["http://dbpedia.org/property/pushpinMap"];
            graph::vmap::iterator itr = G.work.find(resource);
            if (itr == G.work.end()) {
                G.add_vertex(resource,resource);
                G.vertex_add_attribute(resource, "http://dbpedia.org/property/translitLang", node.second->attributes["http://dbpedia.org/property/translitLang"]);
                G.vertex_add_attribute(resource, "~label", "http://dbpedia.org/property/pushpinMap");
            }
            node.second->attributes.erase("http://dbpedia.org/property/pushpinMap");
            node.second->attributes.erase("http://dbpedia.org/property/translitLang");
            GLOBAL_ATTRIBUTE_COUNT--;
            G.add_edge_GNF1(node.first, "http://dbpedia.org/property/pushpinMap", resource);
        }
        if ((point.find("~label") != point.end()) && (point["~label"].compare(":http://dbpedia.org/ontology/MusicGenre") == 0) && (point.find("http://dbpedia.org/property/filename") != point.end()) && (point.find("http://dbpedia.org/property/description") != point.end())) {
            std::string resource = node.second->attributes["http://dbpedia.org/property/filename"];
            graph::vmap::iterator itr = G.work.find(resource);
            if (itr == G.work.end()) {
                G.add_vertex(resource,resource);
                G.vertex_add_attribute(resource, "http://dbpedia.org/property/description", node.second->attributes["http://dbpedia.org/property/description"]);
                G.vertex_add_attribute(resource, "~label", "http://dbpedia.org/property/filename");
            }
            node.second->attributes.erase("http://dbpedia.org/property/filename");
            node.second->attributes.erase("http://dbpedia.org/property/description");
            GLOBAL_ATTRIBUTE_COUNT--;
            G.add_edge_GNF1(node.first, "http://dbpedia.org/property/filename", resource);
        }
        if ((point.find("~label") != point.end()) && (point["~label"].compare(":http://dbpedia.org/ontology/MusicGenre") == 0) && (point.find("http://dbpedia.org/property/quote") != point.end()) && (point.find("http://dbpedia.org/property/culturalOrigins") != point.end()) && (point.find("http://dbpedia.org/property/stylisticOrigins") != point.end()) && (point.find("http://dbpedia.org/property/regionalScenes") != point.end()) && (point.find("http://dbpedia.org/property/source") != point.end())) {
            std::string resource = node.second->attributes["http://dbpedia.org/property/quote"] + "_" + node.second->attributes["http://dbpedia.org/property/culturalOrigins"] + "_" + node.second->attributes["http://dbpedia.org/property/stylisticOrigins"] + "_" + node.second->attributes["http://dbpedia.org/property/regionalScenes"];
            graph::vmap::iterator itr = G.work.find(resource);
            if (itr == G.work.end()) {
                G.add_vertex(resource,resource);
                G.vertex_add_attribute(resource, "http://dbpedia.org/property/quote", node.second->attributes["http://dbpedia.org/property/quote"]);
                G.vertex_add_attribute(resource, "http://dbpedia.org/property/culturalOrigins", node.second->attributes["http://dbpedia.org/property/culturalOrigins"]);
                G.vertex_add_attribute(resource, "http://dbpedia.org/property/stylisticOrigins", node.second->attributes["http://dbpedia.org/property/stylisticOrigins"]);
                G.vertex_add_attribute(resource, "http://dbpedia.org/property/regionalScenes", node.second->attributes["http://dbpedia.org/property/regionalScenes"]);

                G.vertex_add_attribute(resource, "http://dbpedia.org/property/source", node.second->attributes["http://dbpedia.org/property/source"]);
                G.vertex_add_attribute(resource, "~label", "http://dbpedia.org/property/source");
            }
            node.second->attributes.erase("http://dbpedia.org/property/source");
            node.second->attributes.erase("http://dbpedia.org/property/culturalOrigins");
            node.second->attributes.erase("http://dbpedia.org/property/stylisticOrigins");
            node.second->attributes.erase("http://dbpedia.org/property/regionalScenes");
            node.second->attributes.erase("http://dbpedia.org/property/quote");\
            GLOBAL_ATTRIBUTE_COUNT--;
            GLOBAL_ATTRIBUTE_COUNT--;
            GLOBAL_ATTRIBUTE_COUNT--;
            GLOBAL_ATTRIBUTE_COUNT--;
            G.add_edge_GNF1(node.first, "http://dbpedia.org/property/source", resource);
        }
    }
}

void GNF5(graph& G, bool quotemarks) {
    typedef std::map<std::string, int> hist;
    typedef std::map<std::string, hist> histmap;
    typedef std::set<std::string> flagger;
    histmap histogram_map;
    flagger flags;
    histmap::iterator histmap_itr;
    hist::iterator hist_itr;
    flagger::iterator flag_itr;

    for (const auto& node : G.work) {
        for (const auto& attribute_item : node.second->attributes) {
            if ((attribute_item.first.compare("~label") != 0) && (attribute_item.first.compare("\"~label\"") != 0)) {
                histmap_itr = histogram_map.find(attribute_item.first);
                if (histmap_itr == histogram_map.end()) {
                    histogram_map[attribute_item.first][attribute_item.second] = 1;
                } else {
                    hist_itr = histogram_map[attribute_item.first].find(attribute_item.second);
                    if (hist_itr == histogram_map[attribute_item.first].end()) {
                        histogram_map[attribute_item.first][attribute_item.second] = 1;
                    } else {
                        histogram_map[attribute_item.first][attribute_item.second]++;
                        flags.insert(attribute_item.first);
                    }
                }
            }
        }
    }
    graph::vmap old_graph = G.work;
    vertex::att_map::iterator att_itr;
    int CHECKPOINT = 0;
    for (auto const& node : old_graph) {
        CHECKPOINT++;
        vertex::att_map new_att;
        for (auto const& attribute_item :  node.second->attributes) {
            flag_itr = flags.find(attribute_item.first);
            if (flag_itr != flags.end()) {
                graph::vmap::iterator itr = G.work.find(attribute_item.second);
                if (itr == G.work.end()) {
                    G.add_vertex(attribute_item.second,attribute_item.second);
                    if (quotemarks) {
                        G.vertex_add_attribute(attribute_item.second, "~label", attribute_item.first);
                    } else {
                        G.vertex_add_attribute(attribute_item.second, "\"~label\"", attribute_item.first);
                    }
                }
                if (attribute_item.second.compare("Animalia") == 0) {
                    std::cout << "I AM HERE"  << std::endl;
                    std::cout << node.first << std::endl;
                }
                G.add_edge_GNF1(node.first, attribute_item.first, attribute_item.second);
            } else {
                new_att[attribute_item.first] = attribute_item.second;
            }
        }
        G.work[node.first]->attributes = new_att;
    }
}

void related_work_offshore() {
    std::ofstream output_csv;
    output_csv.open ("results/offshore_vldb_results.csv");
    output_csv << "Form, total nodes, total edges, total reduction in redundancy, delta redundancy, delta nodes, delta edges, time(ms) for step, accumalative time(ms),space est, # attributes\n";

    auto start_time = std::chrono::high_resolution_clock::now();
    graph G;
    auto start_load = std::chrono::high_resolution_clock::now();
    bool quotemarks = true;
    bool northwind_label = false;
    bool offshore_label = false;

    readOffshore(G);
    quotemarks = false;

    auto end_load = std::chrono::high_resolution_clock::now();
    auto time_load = end_load - start_load;

    output_csv << "GNF0," << GLOBAL_VERTEX_COUNT << ',' << GLOBAL_EDGES_COUNT <<  ",0,0,0,0," << time_load/std::chrono::milliseconds(1) << ',' << time_load/std::chrono::milliseconds(1) << ',' << 0 << ',' << GLOBAL_ATTRIBUTE_COUNT << "\n";
    int old_edges=GLOBAL_EDGES_COUNT, old_vertices=GLOBAL_VERTEX_COUNT, old_vertex_dupe=GLOBAL_VERTEX_DUPE_COUNT, old_edge_dupe=GLOBAL_EDGE_DUPE_COUNT;
    auto start_NF2 = std::chrono::high_resolution_clock::now();

    GNF3_offshore(G);

    auto end_NF2 = std::chrono::high_resolution_clock::now();
    auto time_NF2 = end_NF2 - start_NF2;
    
    output_csv << "GNF_VLDB," << GLOBAL_VERTEX_COUNT << ',' << GLOBAL_EDGES_COUNT <<  ',' << GLOBAL_VERTEX_DUPE_COUNT + GLOBAL_EDGE_DUPE_COUNT  << ',' << GLOBAL_VERTEX_DUPE_COUNT - old_vertex_dupe + GLOBAL_EDGE_DUPE_COUNT - old_edge_dupe << ',' << GLOBAL_VERTEX_COUNT - old_vertices << ',' << GLOBAL_EDGES_COUNT - old_edges << ',' << time_NF2/std::chrono::milliseconds(1) << ',' << (time_load+time_NF2)/std::chrono::milliseconds(1) << ',' << 0 << ',' << GLOBAL_ATTRIBUTE_COUNT << "\n";

    G.export_to_NEO4j_json("offshore_vldb", quotemarks, northwind_label);
    
}

void related_work_northwind() {
    std::ofstream output_csv;
    output_csv.open ("results/northwind_vldb_results.csv");
    output_csv << "Form, total nodes, total edges, total reduction in redundancy, delta redundancy, delta nodes, delta edges, time(ms) for step, accumalative time(ms),space est, # attributes\n";

    auto start_time = std::chrono::high_resolution_clock::now();
    graph G;
    auto start_load = std::chrono::high_resolution_clock::now();
    bool quotemarks = true;
    bool northwind_label = false;
    bool offshore_label = false;

    readNorthwind(G);
    northwind_label = true;

    auto end_load = std::chrono::high_resolution_clock::now();
    auto time_load = end_load - start_load;

    output_csv << "GNF0," << GLOBAL_VERTEX_COUNT << ',' << GLOBAL_EDGES_COUNT <<  ",0,0,0,0," << time_load/std::chrono::milliseconds(1) << ',' << time_load/std::chrono::milliseconds(1) << ',' << 0 << ',' << GLOBAL_ATTRIBUTE_COUNT << "\n";
    int old_edges=GLOBAL_EDGES_COUNT, old_vertices=GLOBAL_VERTEX_COUNT, old_vertex_dupe=GLOBAL_VERTEX_DUPE_COUNT, old_edge_dupe=GLOBAL_EDGE_DUPE_COUNT;
    auto start_NF2 = std::chrono::high_resolution_clock::now();

    GNF2_northwind(G);

    auto end_NF2 = std::chrono::high_resolution_clock::now();
    auto time_NF2 = end_NF2 - start_NF2;
    
    output_csv << "GNF_VLDB," << GLOBAL_VERTEX_COUNT << ',' << GLOBAL_EDGES_COUNT <<  ',' << GLOBAL_VERTEX_DUPE_COUNT + GLOBAL_EDGE_DUPE_COUNT  << ',' << GLOBAL_VERTEX_DUPE_COUNT - old_vertex_dupe + GLOBAL_EDGE_DUPE_COUNT - old_edge_dupe << ',' << GLOBAL_VERTEX_COUNT - old_vertices << ',' << GLOBAL_EDGES_COUNT - old_edges << ',' << time_NF2/std::chrono::milliseconds(1) << ',' << (time_load+time_NF2)/std::chrono::milliseconds(1) << ',' << 0 << ',' << GLOBAL_ATTRIBUTE_COUNT << "\n";

    G.export_to_NEO4j_json("northwind_vldb", quotemarks, northwind_label);
}

int main(int argc, char *argv[]) {
    

    int NF_level = 0;

    int sub_step_construction = 0;

    std::string dataset = "";

    if (argc < 2) {
        std::cout << "Need to provide a dataset of these: \"dbpedia\", \"dblp\", \"offshore\", \"northwind\"" << std::endl;
        return 1;
    } else {
        dataset = argv[1];
        std::cout << "Dataset: " << dataset << std::endl;
    }

    if (argc >= 3) {
        NF_level = std::atoi(argv[2]);
        std::cout << "NF-Levels: " << NF_level << std::endl;
    }

    if (argc >= 4) {
        sub_step_construction = std::atoi(argv[3]);
        std::cout << "Store every sub-step graph: " << sub_step_construction << std::endl;
    }
    std::cout << std::endl;

    /*
    std::ofstream out("out_dbpedia.txt");
    std::streambuf *coutbuf = std::cout.rdbuf(); //save old buf
    std::cout.rdbuf(out.rdbuf()); //redirect std::cout to out.txt!
    */
    
    if (dataset.compare("vldb_off") == 0) {
        related_work_offshore();
        return 0;
    }
    if (dataset.compare("vldb_north") == 0) {
        related_work_northwind();
        return 0;
    }

    std::ofstream output_csv;
    output_csv.open ("results/"+dataset+"_results.csv");
    output_csv << "Form, total nodes, total edges, total reduction in redundancy, delta redundancy, delta nodes, delta edges, time(ms) for step, accumalative time(ms),space est, # attributes\n";

    auto start_time = std::chrono::high_resolution_clock::now();
    graph G;
    auto start_load = std::chrono::high_resolution_clock::now();
    bool quotemarks = true;
    bool northwind_label = false;
    bool offshore_label = false;

    if (dataset.compare("dbpedia")==0) {
        readDBpedia(G);
    }
    if (dataset.compare("offshore")==0) {
        readOffshore(G);
        quotemarks = false;
    }
    if (dataset.compare("dblp")==0) {
        readDBLP_paper_author(G);
    }
    if (dataset.compare("northwind")==0) {
        readNorthwind(G);
        northwind_label = true;
    }
    if (dataset.compare("go")==0) {
        readGO(G);
    }
    if (dataset.compare("mimic")==0) {
        readMimic(G);
    }

    auto end_load = std::chrono::high_resolution_clock::now();
    auto time_load = end_load - start_load;
    
    // GFD_Miner miner(&G, /*max_fd=*/100, /*time_limit_sec=*/600.0, 3, 3);
    // auto fds = miner.mine();

    // for (auto& fd : fds) {
    //     std::cout << "(";
    //     for (auto& a : fd.lhs_attrs) std::cout << a << ", ";
    //     std::cout << ") -> " << fd.rhs_attr << " over [";
    //     for (auto& p : fd.pattern_labels) std::cout << p << " ";
    //     std::cout << "]\n";
    // }

    std::cout << std::endl;
    std::cout << "Total time for Load: " <<time_load/std::chrono::milliseconds(1) << std::endl;
    std::cout << std::endl;
    output_csv << "GNF0," << GLOBAL_VERTEX_COUNT << ',' << GLOBAL_EDGES_COUNT <<  ",0,0,0,0," << time_load/std::chrono::milliseconds(1) << ',' << time_load/std::chrono::milliseconds(1) << ',' << 0 << ',' << GLOBAL_ATTRIBUTE_COUNT << "\n";
    
    int old_edges=GLOBAL_EDGES_COUNT, old_vertices=GLOBAL_VERTEX_COUNT, old_vertex_dupe=GLOBAL_VERTEX_DUPE_COUNT, old_edge_dupe=GLOBAL_EDGE_DUPE_COUNT;
    if (NF_level == 0) {
        G.export_to_NEO4j_json(dataset+"_0", quotemarks, northwind_label);
        return 0;
    } else if (sub_step_construction) {
        G.export_to_NEO4j_json(dataset+"_0", quotemarks, northwind_label);
    }

    // 1-GNF
    auto start_NF1 = std::chrono::high_resolution_clock::now();

    GNF1(G);
 
    auto end_NF1 = std::chrono::high_resolution_clock::now();
    auto time_NF1 = end_NF1 - start_NF1;
    std::cout << "Total time for NF1: " << time_NF1/std::chrono::milliseconds(1) << std::endl;
    std::cout << std::endl;
    output_csv << "GNF1," << GLOBAL_VERTEX_COUNT << ',' << GLOBAL_EDGES_COUNT <<  ',' << GLOBAL_VERTEX_DUPE_COUNT + GLOBAL_EDGE_DUPE_COUNT  << ',' << GLOBAL_VERTEX_DUPE_COUNT - old_vertex_dupe + GLOBAL_EDGE_DUPE_COUNT - old_edge_dupe << ',' << GLOBAL_VERTEX_COUNT - old_vertices << ',' << GLOBAL_EDGES_COUNT - old_edges << ',' << time_NF1/std::chrono::milliseconds(1) << ',' << (time_load+time_NF1)/std::chrono::milliseconds(1) << ',' << 0 << ',' << GLOBAL_ATTRIBUTE_COUNT << "\n";
    old_edges=GLOBAL_EDGES_COUNT, old_vertices=GLOBAL_VERTEX_COUNT, old_vertex_dupe=GLOBAL_VERTEX_DUPE_COUNT, old_edge_dupe=GLOBAL_EDGE_DUPE_COUNT;
    
    if (NF_level == 1) {
        G.export_to_NEO4j_json(dataset+"_1", quotemarks, northwind_label);
        return 0;
    } else if (sub_step_construction) {
        G.export_to_NEO4j_json(dataset+"_1", quotemarks, northwind_label);
    }

    auto start_NF2 = std::chrono::high_resolution_clock::now();

    if (dataset.compare("dbpedia")==0) {
        GNF2_dbpedia(G);
    }
    if (dataset.compare("offshore")==0) {
        GNF2_offshore(G);
    }
    if (dataset.compare("dblp")==0) {
        GNF2_dblp(G);
    }
    if (dataset.compare("northwind")==0) {
        GNF2_northwind(G);
    }
    if (dataset.compare("go")==0) {
        GNF2_go(G);
    }
    if (dataset.compare("mimic")==0) {
        GNF2_mimic(G);
    }

    auto end_NF2 = std::chrono::high_resolution_clock::now();
    auto time_NF2 = end_NF2 - start_NF2;
    // std::cout << "Moved attributes: " << NF2_MOVED_ATT << std::endl;
    // std::cout << "Attribute redundancy: " << NF2_MOVED_ATT_RED << std::endl;
    std::cout << "Total time for NF2: " << time_NF2/std::chrono::milliseconds(1) << std::endl;
    std::cout << std::endl;
    output_csv << "GNF2," << GLOBAL_VERTEX_COUNT << ',' << GLOBAL_EDGES_COUNT <<  ',' << GLOBAL_VERTEX_DUPE_COUNT + GLOBAL_EDGE_DUPE_COUNT  << ',' << GLOBAL_VERTEX_DUPE_COUNT - old_vertex_dupe + GLOBAL_EDGE_DUPE_COUNT - old_edge_dupe << ',' << GLOBAL_VERTEX_COUNT - old_vertices << ',' << GLOBAL_EDGES_COUNT - old_edges << ',' << time_NF2/std::chrono::milliseconds(1) << ',' << (time_load+time_NF1+time_NF2)/std::chrono::milliseconds(1) << ',' << 0 << ',' << GLOBAL_ATTRIBUTE_COUNT << "\n";
    old_edges=GLOBAL_EDGES_COUNT, old_vertices=GLOBAL_VERTEX_COUNT, old_vertex_dupe=GLOBAL_VERTEX_DUPE_COUNT, old_edge_dupe=GLOBAL_EDGE_DUPE_COUNT;

    if (NF_level == 2) {
        G.export_to_NEO4j_json(dataset+"_2", quotemarks, northwind_label);
        return 0;
    } else if (sub_step_construction) {
        G.export_to_NEO4j_json(dataset+"_2", quotemarks, northwind_label);
    }

    auto start_NF3 = std::chrono::high_resolution_clock::now();

    if (dataset.compare("dbpedia")==0) {
        GNF3_dbpedia(G);
    }
    if (dataset.compare("offshore")==0) {
        GNF3_offshore(G);
    }
    if (dataset.compare("dblp")==0) {
        GNF3_dblp(G);
    }
    if (dataset.compare("northwind")==0) {
        GNF3_northwind(G);
    }
    if (dataset.compare("go")==0) {
        GNF3_go(G);
    }
    if (dataset.compare("mimic")==0) {
        GNF3_mimic(G);
    }

    auto end_NF3 = std::chrono::high_resolution_clock::now();
    auto time_NF3 = end_NF3 - start_NF3;
    // std::cout << "Moved attributes: " << NF3_MOVED_ATT << std::endl;
    // std::cout << "Attribute redundancy: " << NF3_MOVED_ATT_RED << std::endl;
    std::cout << "Total time for NF3: " << time_NF3/std::chrono::milliseconds(1) << std::endl;
    std::cout << std::endl;
    output_csv << "GNF3," << GLOBAL_VERTEX_COUNT << ',' << GLOBAL_EDGES_COUNT <<  ',' << GLOBAL_VERTEX_DUPE_COUNT + GLOBAL_EDGE_DUPE_COUNT  << ',' << GLOBAL_VERTEX_DUPE_COUNT - old_vertex_dupe + GLOBAL_EDGE_DUPE_COUNT - old_edge_dupe << ',' << GLOBAL_VERTEX_COUNT - old_vertices << ',' << GLOBAL_EDGES_COUNT - old_edges << ',' << time_NF3/std::chrono::milliseconds(1) << ',' << (time_load+time_NF1+time_NF2+time_NF3)/std::chrono::milliseconds(1) << ',' << 0 << ',' << GLOBAL_ATTRIBUTE_COUNT << "\n";
    old_edges=GLOBAL_EDGES_COUNT, old_vertices=GLOBAL_VERTEX_COUNT, old_vertex_dupe=GLOBAL_VERTEX_DUPE_COUNT, old_edge_dupe=GLOBAL_EDGE_DUPE_COUNT;

    if (NF_level == 3) {
        G.export_to_NEO4j_json(dataset+"_3", quotemarks, northwind_label);
        return 0;
    } else if (sub_step_construction) {
        G.export_to_NEO4j_json(dataset+"_3", quotemarks, northwind_label);
    }

    auto start_NF4 = std::chrono::high_resolution_clock::now();

    if (dataset.compare("dbpedia")==0) {
        
    }
    if (dataset.compare("offshore")==0) {
        
    }
    if (dataset.compare("dblp")==0) {
        
    }
    if (dataset.compare("northwind")==0) {
        
    }
    if (dataset.compare("go")==0) {
        
    }
    if (dataset.compare("mimic")==0) {
        
    }

    auto end_NF4 = std::chrono::high_resolution_clock::now();
    auto time_NF4 = end_NF4 - start_NF4;
    // std::cout << "Moved attributes: " << NF4_MOVED_ATT << std::endl;
    // std::cout << "Attribute redundancy: " << NF4_MOVED_ATT_RED << std::endl;
    std::cout << "Total time for NF4: " << time_NF4/std::chrono::milliseconds(1) << std::endl;
    std::cout << std::endl;
    output_csv << "GNF4," << GLOBAL_VERTEX_COUNT << ',' << GLOBAL_EDGES_COUNT <<  ',' << GLOBAL_VERTEX_DUPE_COUNT + GLOBAL_EDGE_DUPE_COUNT  << ',' << GLOBAL_VERTEX_DUPE_COUNT - old_vertex_dupe + GLOBAL_EDGE_DUPE_COUNT - old_edge_dupe << ',' << GLOBAL_VERTEX_COUNT - old_vertices << ',' << GLOBAL_EDGES_COUNT - old_edges << ',' << time_NF4/std::chrono::milliseconds(1) << ',' << (time_load+time_NF1+time_NF2+time_NF3+time_NF4)/std::chrono::milliseconds(1) << ',' << 0 << ',' << GLOBAL_ATTRIBUTE_COUNT << "\n";
    old_edges=GLOBAL_EDGES_COUNT, old_vertices=GLOBAL_VERTEX_COUNT, old_vertex_dupe=GLOBAL_VERTEX_DUPE_COUNT, old_edge_dupe=GLOBAL_EDGE_DUPE_COUNT;

    if (NF_level == 4) {
        G.export_to_NEO4j_json(dataset+"_4", quotemarks, northwind_label);
        return 0;
    } else if (sub_step_construction) {
        G.export_to_NEO4j_json(dataset+"_4", quotemarks, northwind_label);
    }

    // E-GNF
    auto start_NF5 = std::chrono::high_resolution_clock::now();

    GNF5(G, quotemarks);

    auto end_NF5 = std::chrono::high_resolution_clock::now();
    auto time_NF5 = end_NF5 - start_NF5;
    std::cout << "Total time for NF5: " << time_NF5/std::chrono::milliseconds(1) << std::endl;
    std::cout << std::endl;
    output_csv << "GNF5," << GLOBAL_VERTEX_COUNT << ',' << GLOBAL_EDGES_COUNT <<  ',' << GLOBAL_VERTEX_DUPE_COUNT + GLOBAL_EDGE_DUPE_COUNT  << ',' << GLOBAL_VERTEX_DUPE_COUNT - old_vertex_dupe + GLOBAL_EDGE_DUPE_COUNT - old_edge_dupe << ',' << GLOBAL_VERTEX_COUNT - old_vertices << ',' << GLOBAL_EDGES_COUNT - old_edges << ',' << time_NF5/std::chrono::milliseconds(1) << ',' << (time_load+time_NF1+time_NF2+time_NF3+time_NF4+time_NF5)/std::chrono::milliseconds(1) << ',' << 0 << ',' << GLOBAL_ATTRIBUTE_COUNT << "\n";
    
    if (NF_level == 5) {
        G.export_to_NEO4j_json(dataset+"_5", quotemarks, northwind_label);
        return 0;
    } else if (sub_step_construction) {
        G.export_to_NEO4j_json(dataset+"_5", quotemarks, northwind_label);
    }

    //END wrap up
    auto end_time = std::chrono::high_resolution_clock::now();
    auto time = end_time - start_time;
    std::cout << "End2End time: " <<time/std::chrono::milliseconds(1) << std::endl;
    output_csv.close();
    return 0; 
}