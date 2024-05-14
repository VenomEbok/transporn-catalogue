#pragma once

#include <iostream>
#include <map>
#include <string>
#include <vector>
#include <variant>
namespace json {
class Node;
struct OstreanNode;
// Сохраните объявления Dict и Array без изменения
using Dict = std::map<std::string, Node>;
using Array = std::vector<Node>;

// Эта ошибка должна выбрасываться при ошибках парсинга JSON
class ParsingError : public std::runtime_error {
public:
    using runtime_error::runtime_error;
};

class Node {
public:
   /* Реализуйте Node, используя std::variant */
    using Value = std::variant<std::nullptr_t, Array, Dict, int, double, std::string, bool>;
    Node()=default;
    Node(std::nullptr_t value);
    Node(Array array);
    Node(Dict map);
    Node(int value);
    Node(std::string value);
    Node(double value);
    Node(bool value);

    const Array& AsArray() const;
    const Dict& AsMap() const;
    int AsInt() const;
    const std::string& AsString() const;
    bool AsBool() const;
    double AsDouble() const;

    bool IsInt() const;
    bool IsDouble() const;
    bool IsPureDouble() const;
    bool IsBool() const;
    bool IsString() const;
    bool IsNull() const;
    bool IsArray() const;
    bool IsMap() const;

    const Value& GetValue() const {
        return node_;
    }

    Value& GetValue() {
        return node_;
    }

   bool operator==(const Node& right) const {
       if (this->node_.index() == right.node_.index()) {
           if (this->node_ == right.node_) {
               return true;
           }
       }
        return false;
    }

   bool operator!=(const Node& right) const {
       if (!(*this == right)) {
           return true;
        }
       return false;
    }
private:
    Value node_;
};

struct OstreamNode {
    std::ostream& out;

    void operator()(const int& value) const {
        out << value;
    }

    void operator()(const double& value) const {
        out << value;
    }

    void operator()(const std::nullptr_t& value [[maybe_unused]] ) const {
        out << "null";
    }

    void operator()(const Array& value) const {
        out << "[\n";
        for (size_t i = 0; i < value.size();++i) {
            std::visit(OstreamNode{ out }, value[i].GetValue());
            if (i + 1 != value.size()) {
                out << ",\n";
            }
        }
        out << "\n]";
    }

    void operator()(const bool& value) const {
        if (value) {
            out << "true";
        }
        else {
            out << "false";
        }
    }

    void operator()(const Dict& value) const {
        bool is_first = true;
        out << "{\n";
        for (auto elem:value) {
            if (!is_first) {
                out << ",\n";
            }
            out <<"\"" << elem.first << "\": ";
            std::visit(OstreamNode{ out }, elem.second.GetValue());
            is_first = false;
        }
        out << "\n}";
    }

    void operator()(const std::string& value) const {
        out << "\"";
        for (const char& c : value) {
            if (c == '\n') {
                out << "\\n";
            }
            else if (c == '\r') {
                out << "\\r";
            }
            else if (c == '\t') {
                out << "\\t";
            }
            else {
                if (c == '\\' || c == '\"') {
                    out << "\\";
                }
                out << c;
            }

        }
        out << "\"";
    }
};

class Document {
public:
    explicit Document(Node root);
    const Node& GetRoot() const;

    bool operator==(const Document& right) {
        return this->GetRoot() == right.GetRoot();
    }

    bool operator!=(const Document& right) {
        return this->GetRoot() != right.GetRoot();
    }
private:
    Node root_;
};

Document Load(std::istream& input);

void Print(const Document& doc, std::ostream& output);

}  // namespace json