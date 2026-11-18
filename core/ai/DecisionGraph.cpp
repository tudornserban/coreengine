#include "core/ai/DecisionGraph.h"

#include <algorithm>
#include <cctype>
#include <cmath>
#include <sstream>
#include <unordered_map>

namespace CoreEngine {
namespace {

std::string JsonEscape(const std::string& s) {
    std::string out;
    out.reserve(s.size() + 8);
    for (char c : s) {
        switch (c) {
        case '"':
            out += "\\\"";
            break;
        case '\\':
            out += "\\\\";
            break;
        case '\n':
            out += "\\n";
            break;
        case '\r':
            out += "\\r";
            break;
        case '\t':
            out += "\\t";
            break;
        default:
            out += c;
            break;
        }
    }
    return out;
}

const char* NodeTypeToString(DecisionNodeType t) {
    switch (t) {
    case DecisionNodeType::Entry:
        return "Entry";
    case DecisionNodeType::Condition:
        return "Condition";
    case DecisionNodeType::Selector:
        return "Selector";
    case DecisionNodeType::Action:
        return "Action";
    }
    return "Entry";
}

bool NodeTypeFromString(const std::string& s, DecisionNodeType* out) {
    if (s == "Entry") {
        *out = DecisionNodeType::Entry;
        return true;
    }
    if (s == "Condition") {
        *out = DecisionNodeType::Condition;
        return true;
    }
    if (s == "Selector") {
        *out = DecisionNodeType::Selector;
        return true;
    }
    if (s == "Action") {
        *out = DecisionNodeType::Action;
        return true;
    }
    return false;
}

const char* TargetToString(DecisionTarget t) {
    switch (t) {
    case DecisionTarget::Self:
        return "Self";
    case DecisionTarget::Enemy:
        return "Enemy";
    case DecisionTarget::World:
        return "World";
    }
    return "Self";
}

bool TargetFromString(const std::string& s, DecisionTarget* out) {
    if (s == "Self") {
        *out = DecisionTarget::Self;
        return true;
    }
    if (s == "Enemy") {
        *out = DecisionTarget::Enemy;
        return true;
    }
    if (s == "World") {
        *out = DecisionTarget::World;
        return true;
    }
    return false;
}

const char* ValueKindToString(DecisionValueKind k) {
    switch (k) {
    case DecisionValueKind::Float:
        return "Float";
    case DecisionValueKind::Int:
        return "Int";
    case DecisionValueKind::Bool:
        return "Bool";
    }
    return "Float";
}

bool ValueKindFromString(const std::string& s, DecisionValueKind* out) {
    if (s == "Float") {
        *out = DecisionValueKind::Float;
        return true;
    }
    if (s == "Int") {
        *out = DecisionValueKind::Int;
        return true;
    }
    if (s == "Bool") {
        *out = DecisionValueKind::Bool;
        return true;
    }
    return false;
}

const char* OpToString(ComparisonOp op) {
    switch (op) {
    case ComparisonOp::LT:
        return "LT";
    case ComparisonOp::LTE:
        return "LTE";
    case ComparisonOp::EQ:
        return "EQ";
    case ComparisonOp::NEQ:
        return "NEQ";
    case ComparisonOp::GTE:
        return "GTE";
    case ComparisonOp::GT:
        return "GT";
    }
    return "LT";
}

bool OpFromString(const std::string& s, ComparisonOp* out) {
    if (s == "LT") {
        *out = ComparisonOp::LT;
        return true;
    }
    if (s == "LTE") {
        *out = ComparisonOp::LTE;
        return true;
    }
    if (s == "EQ") {
        *out = ComparisonOp::EQ;
        return true;
    }
    if (s == "NEQ") {
        *out = ComparisonOp::NEQ;
        return true;
    }
    if (s == "GTE") {
        *out = ComparisonOp::GTE;
        return true;
    }
    if (s == "GT") {
        *out = ComparisonOp::GT;
        return true;
    }
    return false;
}

const char* SelectorModeToString(SelectorMode m) {
    switch (m) {
    case SelectorMode::Cycle:
        return "Cycle";
    case SelectorMode::Weighted:
        return "Weighted";
    }
    return "Cycle";
}

bool SelectorModeFromString(const std::string& s, SelectorMode* out) {
    if (s == "Cycle") {
        *out = SelectorMode::Cycle;
        return true;
    }
    if (s == "Weighted") {
        *out = SelectorMode::Weighted;
        return true;
    }
    return false;
}

struct JsonObjectRecord {
    std::unordered_map<std::string, std::string> fields;
    std::unordered_map<std::string, std::vector<float>> floatArrays;
};

class JsonReader {
public:
    explicit JsonReader(const std::string& text) : text_(text) {}

    bool ParseDocument(std::string* outError) {
        SkipWs();
        if (!Match('{')) {
            if (outError) {
                *outError = "expected root object";
            }
            return false;
        }
        SkipWs();
        if (Match('}')) {
            return true;
        }
        for (;;) {
            std::string key;
            if (!ParseString(&key, outError)) {
                return false;
            }
            if (!Match(':')) {
                if (outError) {
                    *outError = "expected ':' after key";
                }
                return false;
            }
            SkipWs();
            if (key == "nodes") {
                if (!ParseArrayOfObjectRecords(&nodes_, outError)) {
                    return false;
                }
            } else if (key == "connections") {
                if (!ParseArrayOfObjectRecords(&connections_, outError)) {
                    return false;
                }
            } else if (!SkipValue(outError)) {
                return false;
            }
            SkipWs();
            if (Match('}')) {
                break;
            }
            if (!Match(',')) {
                if (outError) {
                    *outError = "expected ',' or '}' in root object";
                }
                return false;
            }
        }
        SkipWs();
        if (pos_ != text_.size()) {
            if (outError) {
                *outError = "trailing JSON content";
            }
            return false;
        }
        return true;
    }

    const std::vector<JsonObjectRecord>& Nodes() const { return nodes_; }
    const std::vector<JsonObjectRecord>& Connections() const { return connections_; }

private:
    const std::string& text_;
    std::size_t pos_ = 0;
    std::vector<JsonObjectRecord> nodes_;
    std::vector<JsonObjectRecord> connections_;

    void SkipWs() {
        while (pos_ < text_.size() && std::isspace(static_cast<unsigned char>(text_[pos_]))) {
            ++pos_;
        }
    }

    bool Match(char c) {
        SkipWs();
        if (pos_ >= text_.size() || text_[pos_] != c) {
            return false;
        }
        ++pos_;
        return true;
    }

    bool ParseString(std::string* out, std::string* outError) {
        SkipWs();
        if (pos_ >= text_.size() || text_[pos_] != '"') {
            if (outError) {
                *outError = "expected string";
            }
            return false;
        }
        ++pos_;
        out->clear();
        while (pos_ < text_.size()) {
            const char c = text_[pos_++];
            if (c == '"') {
                return true;
            }
            if (c == '\\' && pos_ < text_.size()) {
                const char e = text_[pos_++];
                switch (e) {
                case '"':
                    *out += '"';
                    break;
                case '\\':
                    *out += '\\';
                    break;
                case 'n':
                    *out += '\n';
                    break;
                case 'r':
                    *out += '\r';
                    break;
                case 't':
                    *out += '\t';
                    break;
                default:
                    *out += e;
                    break;
                }
                continue;
            }
            *out += c;
        }
        if (outError) {
            *outError = "unterminated string";
        }
        return false;
    }

    bool ParseNumber(double* out, std::string* outError) {
        SkipWs();
        const std::size_t start = pos_;
        if (pos_ < text_.size() && (text_[pos_] == '-' || text_[pos_] == '+')) {
            ++pos_;
        }
        while (pos_ < text_.size() && std::isdigit(static_cast<unsigned char>(text_[pos_]))) {
            ++pos_;
        }
        if (pos_ < text_.size() && text_[pos_] == '.') {
            ++pos_;
            while (pos_ < text_.size() && std::isdigit(static_cast<unsigned char>(text_[pos_]))) {
                ++pos_;
            }
        }
        if (pos_ == start) {
            if (outError) {
                *outError = "expected number";
            }
            return false;
        }
        try {
            *out = std::stod(text_.substr(start, pos_ - start));
        } catch (...) {
            if (outError) {
                *outError = "invalid number";
            }
            return false;
        }
        return true;
    }

    bool ParseBool(bool* out, std::string* outError) {
        SkipWs();
        if (text_.compare(pos_, 4, "true") == 0) {
            pos_ += 4;
            *out = true;
            return true;
        }
        if (text_.compare(pos_, 5, "false") == 0) {
            pos_ += 5;
            *out = false;
            return true;
        }
        if (outError) {
            *outError = "expected boolean";
        }
        return false;
    }

    bool ParseLiteralNull(std::string* outError) {
        SkipWs();
        if (text_.compare(pos_, 4, "null") == 0) {
            pos_ += 4;
            return true;
        }
        if (outError) {
            *outError = "expected null";
        }
        return false;
    }

    bool SkipValue(std::string* outError) {
        SkipWs();
        if (pos_ >= text_.size()) {
            if (outError) {
                *outError = "unexpected end of JSON";
            }
            return false;
        }
        const char c = text_[pos_];
        if (c == '{') {
            JsonObjectRecord dummy;
            return ParseObjectRecord(&dummy, outError);
        }
        if (c == '[') {
            return ParseArraySkip(outError);
        }
        if (c == '"') {
            std::string s;
            return ParseString(&s, outError);
        }
        if (c == 't' || c == 'f') {
            bool b = false;
            return ParseBool(&b, outError);
        }
        if (c == 'n') {
            return ParseLiteralNull(outError);
        }
        double n = 0;
        return ParseNumber(&n, outError);
    }

    bool ParseObjectRecord(JsonObjectRecord* out, std::string* outError) {
        if (!Match('{')) {
            if (outError) {
                *outError = "expected '{'";
            }
            return false;
        }
        SkipWs();
        if (Match('}')) {
            *out = {};
            return true;
        }
        for (;;) {
            std::string key;
            if (!ParseString(&key, outError)) {
                return false;
            }
            if (!Match(':')) {
                if (outError) {
                    *outError = "expected ':' after key";
                }
                return false;
            }
            SkipWs();
            if (key == "weights" && pos_ < text_.size() && text_[pos_] == '[') {
                std::vector<float> weights;
                if (!ParseFloatArray(&weights, outError)) {
                    return false;
                }
                out->floatArrays[key] = std::move(weights);
            } else if (pos_ < text_.size() && text_[pos_] == '"') {
                std::string strVal;
                if (!ParseString(&strVal, outError)) {
                    return false;
                }
                out->fields[key] = strVal;
            } else if (pos_ < text_.size() && (text_[pos_] == 't' || text_[pos_] == 'f')) {
                bool b = false;
                if (!ParseBool(&b, outError)) {
                    return false;
                }
                out->fields[key] = b ? "true" : "false";
            } else {
                double n = 0;
                if (!ParseNumber(&n, outError)) {
                    return false;
                }
                out->fields[key] = std::to_string(n);
            }
            SkipWs();
            if (Match('}')) {
                break;
            }
            if (!Match(',')) {
                if (outError) {
                    *outError = "expected ',' or '}' in object";
                }
                return false;
            }
        }
        return true;
    }

    bool ParseFloatArray(std::vector<float>* out, std::string* outError) {
        if (!Match('[')) {
            if (outError) {
                *outError = "expected '['";
            }
            return false;
        }
        SkipWs();
        if (Match(']')) {
            out->clear();
            return true;
        }
        for (;;) {
            double n = 0;
            if (!ParseNumber(&n, outError)) {
                return false;
            }
            out->push_back(static_cast<float>(n));
            SkipWs();
            if (Match(']')) {
                return true;
            }
            if (!Match(',')) {
                if (outError) {
                    *outError = "expected ',' or ']' in float array";
                }
                return false;
            }
        }
    }

    bool ParseArraySkip(std::string* outError) {
        if (!Match('[')) {
            return false;
        }
        SkipWs();
        if (Match(']')) {
            return true;
        }
        for (;;) {
            if (!SkipValue(outError)) {
                return false;
            }
            SkipWs();
            if (Match(']')) {
                return true;
            }
            if (!Match(',')) {
                if (outError) {
                    *outError = "expected ',' or ']' in array";
                }
                return false;
            }
        }
    }

    bool ParseArrayOfObjectRecords(std::vector<JsonObjectRecord>* out, std::string* outError) {
        if (!Match('[')) {
            if (outError) {
                *outError = "expected '['";
            }
            return false;
        }
        SkipWs();
        if (Match(']')) {
            out->clear();
            return true;
        }
        for (;;) {
            JsonObjectRecord record;
            if (!ParseObjectRecord(&record, outError)) {
                return false;
            }
            out->push_back(std::move(record));
            SkipWs();
            if (Match(']')) {
                return true;
            }
            if (!Match(',')) {
                if (outError) {
                    *outError = "expected ',' or ']' in array";
                }
                return false;
            }
        }
    }
};

int ParseIntField(const JsonObjectRecord& obj, const std::string& key, int defaultValue) {
    const auto it = obj.fields.find(key);
    if (it == obj.fields.end()) {
        return defaultValue;
    }
    return std::stoi(it->second);
}

float ParseFloatField(const JsonObjectRecord& obj, const std::string& key, float defaultValue) {
    const auto it = obj.fields.find(key);
    if (it == obj.fields.end()) {
        return defaultValue;
    }
    return std::stof(it->second);
}

bool ParseBoolField(const JsonObjectRecord& obj, const std::string& key, bool defaultValue) {
    const auto it = obj.fields.find(key);
    if (it == obj.fields.end()) {
        return defaultValue;
    }
    return it->second == "true" || it->second == "1";
}

std::string GetField(const JsonObjectRecord& obj, const std::string& key) {
    const auto it = obj.fields.find(key);
    if (it == obj.fields.end()) {
        return {};
    }
    return it->second;
}

} // namespace

void DecisionGraph::Clear() {
    nodes_.clear();
    connections_.clear();
}

void DecisionGraph::AddNode(DecisionNode node) {
    nodes_.push_back(std::move(node));
}

const DecisionNode* DecisionGraph::FindNode(const std::string& id) const {
    for (const DecisionNode& node : nodes_) {
        if (node.id == id) {
            return &node;
        }
    }
    return nullptr;
}

bool DecisionGraph::ValidateConnection(const std::string& fromId, int fromPort, const std::string& toId, int toPort,
                                       std::string* outError) const {
    (void)fromPort;
    (void)toPort;
    if (fromId == toId) {
        if (outError) {
            *outError = "self-loop connection is not allowed";
        }
        return false;
    }
    const DecisionNode* fromNode = FindNode(fromId);
    const DecisionNode* toNode = FindNode(toId);
    if (!fromNode || !toNode) {
        if (outError) {
            *outError = "unknown node id in connection";
        }
        return false;
    }
    if (fromNode->type == DecisionNodeType::Action && toNode->type == DecisionNodeType::Action) {
        if (outError) {
            *outError = "Action to Action connection is not allowed";
        }
        return false;
    }
    return true;
}

bool DecisionGraph::AddConnection(const DecisionConnection& connection, std::string* outError) {
    if (!ValidateConnection(connection.fromNodeId, connection.fromPort, connection.toNodeId, connection.toPort,
                           outError)) {
        return false;
    }
    for (DecisionConnection& existing : connections_) {
        if (existing.fromNodeId == connection.fromNodeId && existing.fromPort == connection.fromPort) {
            existing = connection;
            return true;
        }
    }
    connections_.push_back(connection);
    return true;
}

std::string DecisionGraph::FindEntryNodeId() const {
    for (const DecisionNode& node : nodes_) {
        if (node.type == DecisionNodeType::Entry) {
            return node.id;
        }
    }
    return {};
}

const DecisionConnection* DecisionGraph::FindOutgoingConnection(const std::string& fromNodeId, int fromPort) const {
    for (const DecisionConnection& conn : connections_) {
        if (conn.fromNodeId == fromNodeId && conn.fromPort == fromPort) {
            return &conn;
        }
    }
    return nullptr;
}

std::vector<int> DecisionGraph::GetSortedConnectedOutPorts(const std::string& fromNodeId) const {
    std::vector<int> ports;
    for (const DecisionConnection& conn : connections_) {
        if (conn.fromNodeId == fromNodeId) {
            ports.push_back(conn.fromPort);
        }
    }
    std::sort(ports.begin(), ports.end());
    ports.erase(std::unique(ports.begin(), ports.end()), ports.end());
    return ports;
}

std::string DecisionGraph::ToJsonString() const {
    std::ostringstream oss;
    oss << "{\"version\":1,\"nodes\":[";
    for (std::size_t i = 0; i < nodes_.size(); ++i) {
        if (i > 0) {
            oss << ',';
        }
        const DecisionNode& n = nodes_[i];
        oss << '{';
        oss << "\"id\":\"" << JsonEscape(n.id) << "\",";
        oss << "\"type\":\"" << NodeTypeToString(n.type) << "\"";
        if (n.type == DecisionNodeType::Condition) {
            oss << ",\"target\":\"" << TargetToString(n.target) << "\"";
            oss << ",\"valueKind\":\"" << ValueKindToString(n.valueKind) << "\"";
            oss << ",\"propertyKey\":\"" << JsonEscape(n.propertyKey) << "\"";
            oss << ",\"op\":\"" << OpToString(n.op) << "\"";
            oss << ",\"floatThreshold\":" << n.floatThreshold;
            oss << ",\"intThreshold\":" << n.intThreshold;
            oss << ",\"boolThreshold\":" << (n.boolThreshold ? "true" : "false");
        } else if (n.type == DecisionNodeType::Selector) {
            oss << ",\"selectorMode\":\"" << SelectorModeToString(n.selectorMode) << "\"";
            oss << ",\"weights\":[";
            for (std::size_t w = 0; w < n.weights.size(); ++w) {
                if (w > 0) {
                    oss << ',';
                }
                oss << n.weights[w];
            }
            oss << ']';
        } else if (n.type == DecisionNodeType::Action) {
            oss << ",\"primaryActionId\":" << n.primaryActionId;
            oss << ",\"secondaryActionId\":" << n.secondaryActionId;
            oss << ",\"actionTag\":\"" << JsonEscape(n.actionTag) << "\"";
        }
        oss << '}';
    }
    oss << "],\"connections\":[";
    for (std::size_t i = 0; i < connections_.size(); ++i) {
        if (i > 0) {
            oss << ',';
        }
        const DecisionConnection& c = connections_[i];
        oss << '{';
        oss << "\"fromNodeId\":\"" << JsonEscape(c.fromNodeId) << "\",";
        oss << "\"fromPort\":" << c.fromPort << ',';
        oss << "\"toNodeId\":\"" << JsonEscape(c.toNodeId) << "\",";
        oss << "\"toPort\":" << c.toPort;
        oss << '}';
    }
    oss << "]}";
    return oss.str();
}

bool DecisionGraph::FromJsonString(const std::string& json, DecisionGraph* outGraph, std::string* outError) {
    if (outGraph == nullptr) {
        if (outError) {
            *outError = "outGraph is null";
        }
        return false;
    }
    JsonReader reader(json);
    if (!reader.ParseDocument(outError)) {
        return false;
    }

    DecisionGraph graph;
    for (const JsonObjectRecord& obj : reader.Nodes()) {
        DecisionNode node;
        node.id = GetField(obj, "id");
        if (node.id.empty()) {
            if (outError) {
                *outError = "node missing id";
            }
            return false;
        }
        const std::string typeStr = GetField(obj, "type");
        if (!NodeTypeFromString(typeStr, &node.type)) {
            if (outError) {
                *outError = "invalid node type";
            }
            return false;
        }
        if (node.type == DecisionNodeType::Condition) {
            const std::string targetStr = GetField(obj, "target");
            const std::string kindStr = GetField(obj, "valueKind");
            const std::string opStr = GetField(obj, "op");
            if (!TargetFromString(targetStr, &node.target) || !ValueKindFromString(kindStr, &node.valueKind) ||
                !OpFromString(opStr, &node.op)) {
                if (outError) {
                    *outError = "invalid condition field";
                }
                return false;
            }
            node.propertyKey = GetField(obj, "propertyKey");
            node.floatThreshold = ParseFloatField(obj, "floatThreshold", 0.0f);
            node.intThreshold = ParseIntField(obj, "intThreshold", 0);
            node.boolThreshold = ParseBoolField(obj, "boolThreshold", false);
        } else if (node.type == DecisionNodeType::Selector) {
            const std::string modeStr = GetField(obj, "selectorMode");
            if (!SelectorModeFromString(modeStr, &node.selectorMode)) {
                if (outError) {
                    *outError = "invalid selectorMode";
                }
                return false;
            }
            const auto weightsIt = obj.floatArrays.find("weights");
            if (weightsIt != obj.floatArrays.end()) {
                node.weights = weightsIt->second;
            }
        } else if (node.type == DecisionNodeType::Action) {
            node.primaryActionId = ParseIntField(obj, "primaryActionId", 0);
            node.secondaryActionId = ParseIntField(obj, "secondaryActionId", -1);
            node.actionTag = GetField(obj, "actionTag");
        }
        graph.AddNode(std::move(node));
    }

    for (const JsonObjectRecord& obj : reader.Connections()) {
        DecisionConnection conn;
        conn.fromNodeId = GetField(obj, "fromNodeId");
        conn.toNodeId = GetField(obj, "toNodeId");
        conn.fromPort = ParseIntField(obj, "fromPort", 0);
        conn.toPort = ParseIntField(obj, "toPort", 0);
        std::string addErr;
        if (!graph.AddConnection(conn, &addErr)) {
            if (outError) {
                *outError = addErr;
            }
            return false;
        }
    }

    *outGraph = std::move(graph);
    return true;
}

} // namespace CoreEngine