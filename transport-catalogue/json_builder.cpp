#include "json_builder.h"
namespace json {
//-----------------Builder------------------------------------------------
Builder::Builder()
{
	root_ = nullptr;
	nodes_stack_.emplace_back(&root_);
}

Builder::DictValueContext Builder::Key(std::string key)
{
	if (nodes_stack_.empty()) {
		throw std::logic_error("Object already done");
	}
	if (!std::holds_alternative<Dict>(nodes_stack_.back()->GetValue())) {
		throw std::logic_error("No start dict before key");
	}
	nodes_stack_.push_back(&std::get<Dict>(nodes_stack_.back()->GetValue())[std::move(key)]);
	return BaseContext(*this);
}

Builder::BaseContext Builder::Value(Node::Value value)
{
	if (nodes_stack_.empty()) {
		throw std::logic_error("Object already done");
	}
	if (std::holds_alternative<Array>(nodes_stack_.back()->GetValue())) {
		Node elem;
		elem.GetValue() = value;
		std::get<Array>(nodes_stack_.back()->GetValue()).push_back(elem);
		return BaseContext(*this);
	}
	if (std::holds_alternative<std::nullptr_t>(nodes_stack_.back()->GetValue())) {
		nodes_stack_.back()->GetValue() = value;
		nodes_stack_.pop_back();
		return BaseContext(*this);
	}
	throw std::logic_error("Error space for value");
}

Builder::DictItemContext Builder::StartDict()
{	
	if (nodes_stack_.empty()) {
		throw std::logic_error("Object already done");
	}
	if (std::holds_alternative<std::nullptr_t>(nodes_stack_.back()->GetValue())) {
		nodes_stack_.back()->GetValue() = Dict{};
		return BaseContext(*this);
	}
	else if (std::holds_alternative<Array>(nodes_stack_.back()->GetValue())) {
		auto& emplaced = std::get<Array>(nodes_stack_.back()->GetValue()).emplace_back(Dict{});
		nodes_stack_.emplace_back(&emplaced);
		return BaseContext(*this);
	}
	throw std::logic_error("Wrong Dict context");
}

Builder::ArrayItemContext Builder::StartArray()
{
	if (nodes_stack_.empty()) {
		throw std::logic_error("Object already done");
	}
	if (std::holds_alternative<std::nullptr_t>(nodes_stack_.back()->GetValue())) {
		nodes_stack_.back()->GetValue() = Array{};
		return BaseContext(*this);
	}
	else if (std::holds_alternative<Array>(nodes_stack_.back()->GetValue())) {
		auto& emplaced = std::get<Array>(nodes_stack_.back()->GetValue()).emplace_back(Array{});
		nodes_stack_.emplace_back(&emplaced);
		return BaseContext(*this);
	}
	throw std::logic_error("Wrong Array context");
}

Builder::BaseContext Builder::EndDict()
{
	if (nodes_stack_.empty()) {
		throw std::logic_error("Object already done");
	}
	if (!std::holds_alternative<Dict>(nodes_stack_.back()->GetValue())) {
		throw std::logic_error("Wrong Dict context");
	}
	nodes_stack_.pop_back();
	return *this;
}

Builder::BaseContext Builder::EndArray()
{
	if (nodes_stack_.empty()) {
		throw std::logic_error("Object already done");
	}
	if (!std::holds_alternative<Array>(nodes_stack_.back()->GetValue())) {
		throw std::logic_error("Wrong Array context");
	}
	nodes_stack_.pop_back();
	return *this;
}

Node Builder::Build()
{
	if (!nodes_stack_.empty()) {
		throw std::logic_error("Object not ready to build");
	}
	return root_;
}

//-------------------------BaseContext------------------------------------

Builder::BaseContext::BaseContext(Builder& builder):builder_(builder){
}

Node Builder::BaseContext::Build()
{
	return builder_.Build();
}

Builder::DictValueContext Builder::BaseContext::Key(std::string key)
{
	return builder_.Key(std::move(key));
}

Builder::BaseContext Builder::BaseContext::Value(Node::Value value)
{
	return builder_.Value(std::move(value));
}

Builder::DictItemContext Builder::BaseContext::StartDict()
{
	return builder_.StartDict();
}

Builder::ArrayItemContext Builder::BaseContext::StartArray()
{
	return builder_.StartArray();
}

Builder::BaseContext Builder::BaseContext::EndDict()
{
	return builder_.EndDict();
}

Builder::BaseContext Builder::BaseContext::EndArray()
{
	return builder_.EndArray();
}

//-----------------------------DictValueContext----------------------------------------

Builder::DictValueContext::DictValueContext(BaseContext base) : BaseContext(base){
}

//--------------------------DictItemContext--------------------------------------------

Builder::DictItemContext Builder::DictValueContext::Value(Node::Value value)
{
		return BaseContext(*this).Value(std::move(value));

}

Builder::DictItemContext::DictItemContext(BaseContext base):BaseContext(base){
}

//------------------------ArratItemContext-------------------------------------------------

Builder::ArrayItemContext::ArrayItemContext(BaseContext base):BaseContext(base){
}

Builder::ArrayItemContext Builder::ArrayItemContext::Value(Node::Value value){
	return BaseContext(*this).Value(value);
}

}