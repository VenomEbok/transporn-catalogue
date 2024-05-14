#include "json.h"

using namespace std;

namespace json {

namespace {

Node LoadNode(istream& input);

Node LoadString(std::istream& input) {
    using namespace std::literals;

    auto it = std::istreambuf_iterator<char>(input);
    auto end = std::istreambuf_iterator<char>();
    std::string s;
    while (true) {
        if (it == end) {
            // Поток закончился до того, как встретили закрывающую кавычку?
            throw ParsingError("String parsing error");
        }
        const char ch = *it;
        if (ch == '"') {
            // Встретили закрывающую кавычку
            ++it;
            break;
        }
        else if (ch == '\\') {
            // Встретили начало escape-последовательности
            ++it;
            if (it == end) {
                // Поток завершился сразу после символа обратной косой черты
                throw ParsingError("String parsing error");
            }
            const char escaped_char = *(it);
            // Обрабатываем одну из последовательностей: \\, \n, \t, \r, \"
            switch (escaped_char) {
            case 'n':
                s.push_back('\n');
                break;
            case 't':
                s.push_back('\t');
                break;
            case 'r':
                s.push_back('\r');
                break;
            case '"':
                s.push_back('"');
                break;
            case '\\':
                s.push_back('\\');
                break;
            default:
                // Встретили неизвестную escape-последовательность
                throw ParsingError("Unrecognized escape sequence \\"s + escaped_char);
            }
        }
        else if (ch == '\n' || ch == '\r') {
            // Строковый литерал внутри- JSON не может прерываться символами \r или \n
            throw ParsingError("Unexpected end of line"s);
        }
        else {
            // Просто считываем очередной символ и помещаем его в результирующую строку
            s.push_back(ch);
        }
        ++it;
    }

    return Node{ s };
}

using Number = std::variant<int, double>;

Node LoadNumber(std::istream& input) {
    using namespace std::literals;

    std::string parsed_num;

    // Считывает в parsed_num очередной символ из input
    auto read_char = [&parsed_num, &input] {
        parsed_num += static_cast<char>(input.get());
        if (!input) {
            throw ParsingError("Failed to read number from stream"s);
        }
        };

    // Считывает одну или более цифр в parsed_num из input
    auto read_digits = [&input, read_char] {
        if (!std::isdigit(input.peek())) {
            throw ParsingError("A digit is expected"s);
        }
        while (std::isdigit(input.peek())) {
            read_char();
        }
        };

    if (input.peek() == '-') {
        read_char();
    }
    // Парсим целую часть числа
    if (input.peek() == '0') {
        read_char();
        // После 0 в JSON не могут идти другие цифры
    }
    else {
        read_digits();
    }

    bool is_int = true;
    // Парсим дробную часть числа
    if (input.peek() == '.') {
        read_char();
        read_digits();
        is_int = false;
    }

    // Парсим экспоненциальную часть числа
    if (int ch = input.peek(); ch == 'e' || ch == 'E') {
        read_char();
        if (ch = input.peek(); ch == '+' || ch == '-') {
            read_char();
        }
        read_digits();
        is_int = false;
    }

    try {
        if (is_int) {
            // Сначала пробуем преобразовать строку в int
            try {
                return Node(std::stoi(parsed_num));
            }
            catch (...) {
                // В случае неудачи, например, при переполнении,
                // код ниже попробует преобразовать строку в double
            }
        }
        return Node(std::stod(parsed_num));
    }
    catch (...) {
        throw ParsingError("Failed to convert "s + parsed_num + " to number"s);
    }
}

Node LoadArray(istream& input) {
    Array result;
    char c;
    for (; input >> c && c != ']';) {
        if (c != ',') {
            input.putback(c);
        }
        result.push_back(LoadNode(input));
    }
    if (c != ']') {
        throw ParsingError("No ]");
    }
    return Node(move(result));
}


Node LoadDict(istream& input) {
    Dict result;
    char c;
    for (; input >> c && c != '}';) {
        if (c == ',') {
            input >> c;
        }

        string key = LoadString(input).AsString();
        input >> c;
        result.insert({move(key), LoadNode(input)});
    }
    if (c != '}') {
        throw ParsingError("No }");
    }
    return Node(move(result));
}

Node LoadOther(istream& input) {
    string str;
    char c;
    while ((c = static_cast<char>(input.get())) && std::isalpha(c))
    {
        str += c;
    }
    if (!std::isalpha(c))
    {
        input.putback(c);
    }
    if (str == "null" || str == "nullptr")
    {
        return Node(nullptr);
    }
    if (str == "true" || str == "false")
    {
        return Node(str == "true");
    }
    throw ParsingError("Unknown type"s);
}

Node LoadNode(istream& input) {
    char c;
    input >> c;

    if (c == '[') {
        return LoadArray(input);
    } else if (c == '{') {
        //input.putback(c);
        return LoadDict(input);
    } else if (c == '"') {
        //input.putback(c);
        return LoadString(input);
    } else if(isdigit(c) || c=='-'||c=='+') {
        input.putback(c);
        return LoadNumber(input);
    }
    else if (isalpha(c)) {
        input.putback(c);
        return LoadOther(input);
    }
    return Node();

}

}  // namespace

Node::Node(std::nullptr_t value)
    :node_(move(value)){
}

Node::Node(Array array)
    : node_(move(array)) {
}

Node::Node(Dict map)
    : node_(move(map)) {
}

Node::Node(int value)
    : node_(value) {
}

Node::Node(string value)
    : node_(move(value)) {
}

Node::Node(double value)
    : node_(move(value)){
}

Node::Node(bool value)
    :node_(move(value)){
}

const Array& Node::AsArray() const {
    if (std::holds_alternative<Array>(node_)) {
        return std::get<Array>(node_);
    }
    throw std::logic_error("Not array");

}

const Dict& Node::AsMap() const {
    if (std::holds_alternative<Dict>(node_)) {
        return std::get<Dict>(node_);
    }
    throw std::logic_error("Not map");
}

int Node::AsInt() const {
    if (std::holds_alternative<int>(node_)) {
        return std::get<int>(node_);
    }
    throw std::logic_error("Not int");
}

const string& Node::AsString() const {
    if (std::holds_alternative<std::string>(node_)) {
        return std::get<std::string>(node_);
    }
    throw std::logic_error("Not string");
}

bool Node::AsBool() const {
    if (std::holds_alternative<bool>(node_)) {
        return std::get<bool>(node_);
    }
    throw std::logic_error("Not bool");
}

double Node::AsDouble() const{
    if (std::holds_alternative<double>(node_)) {
        return std::get<double>(node_);
    }
    else if (std::holds_alternative<int>(node_)) {
        return static_cast<double>(std::get<int>(node_));
    }
    throw std::logic_error("Not double");
}

bool Node::IsInt() const{
    if (std::holds_alternative<int>(node_)) {
        return true;
    }
    return false;
}

bool Node::IsDouble() const{
    if (std::holds_alternative<int>(node_)||std::holds_alternative<double>(node_)) {
        return true;
    }
    return false;
}

bool Node::IsPureDouble() const{
    if (std::holds_alternative<double>(node_)) {
        return true;
    }
    return false;
}

bool Node::IsBool() const{
    if (std::holds_alternative<bool>(node_)) {
        return true;
    }
    return false;
}

bool Node::IsString() const{
    if (std::holds_alternative<std::string>(node_)) {
        return true;
    }
    return false;
}

bool Node::IsNull() const{
    if (std::holds_alternative<std::nullptr_t>(node_)) {
        return true;
    }
    return false;
}

bool Node::IsArray() const{
    if (std::holds_alternative<Array>(node_)) {
        return true;
    }
    return false;
}

bool Node::IsMap() const{
    if (std::holds_alternative<Dict>(node_)) {
        return true;
    }
    return false;
}

Document::Document(Node root)
    : root_(move(root)) {
}

const Node& Document::GetRoot() const {
    return root_;
}

Document Load(istream& input) {
    return Document{LoadNode(input)};
}

void Print(const Document& doc, std::ostream& output) {
    if (doc.GetRoot().IsNull()) {
        std::visit(OstreamNode{ output }, doc.GetRoot().GetValue());
    }else if(doc.GetRoot().IsInt()||doc.GetRoot().IsDouble()){
        std::visit(OstreamNode{ output }, doc.GetRoot().GetValue());
    }
    else {
        std::visit(OstreamNode{ output }, doc.GetRoot().GetValue());
    }
    
}

}  // namespace json