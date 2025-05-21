#pragma once


#include <vector>
#include <cstdio>

#include "xmltoken.h"


namespace waavs {
    struct JsonElement {
        ByteSpan tagName;
        bool hasChildren = false;
    };

    static void jsonEscaped(const ByteSpan& span, FILE* out = stdout) {
        fwrite("\"", 1, 1, out);
        for (const unsigned char* p = span.fStart; p < span.fEnd; ++p) {
            char c = *p;
            if (c == '"') fputs("\\\"", out);
            else if (c == '\\') fputs("\\\\", out);
            else if (c == '\n') fputs("\\n", out);
            else if (c == '\r') fputs("\\r", out);
            else if (c == '\t') fputs("\\t", out);
            else fwrite(p, 1, 1, out);
        }
        fwrite("\"", 1, 1, out);
    }

    static void printXmlToJson(const ByteSpan& src, FILE* out = stdout, bool collapseWhitespace=true)
    {
        XmlTokenGenerator xmlGen(src);
        XmlToken tok;

        std::vector<JsonElement> stack;

        bool needComma = false;

		// simple lambda to handle indentation
        auto indent = [&](int depth) {
            for (int i = 0; i < depth; ++i)
                fputs("  ", out);
            };

        // repeatedly pull xml tokens out of the byte stream
        while (xmlGen.next(tok)) {
            switch (tok.type) {
            case XML_TOKEN_LT: {

                XmlToken nxtoken;
                if (!xmlGen.next(nxtoken))
                    return;

                if (nxtoken.type == XML_TOKEN_SLASH) {
                    // End tag
                    xmlGen.next(nxtoken); // tag name
                    xmlGen.next(nxtoken); // >
                    if (!stack.empty()) {
                        indent(stack.size() - 1);
                        fputs("] }", out);
                        stack.pop_back();
                        needComma = true;
                    }
                }
                else if (nxtoken.type == XML_TOKEN_NAME) {
                    // Start or self-closing tag
                    ByteSpan tagName = nxtoken.value;
                    std::vector<std::pair<ByteSpan, ByteSpan>> attrs;
                    bool selfClosing = false;

                    // Consume attributes
                    while (xmlGen.next(nxtoken)) {
                        if (nxtoken.type == XML_TOKEN_GT) {
                            break;
                        }
                        else if (nxtoken.type == XML_TOKEN_SLASH) {
                            xmlGen.next(nxtoken); // should be GT
                            selfClosing = true;
                            break;
                        }
                        else if (nxtoken.type == XML_TOKEN_NAME) {
                            ByteSpan attrName = nxtoken.value;
                            if (!xmlGen.next(nxtoken) || nxtoken.type != XML_TOKEN_EQ) break;
                            if (!xmlGen.next(nxtoken) || nxtoken.type != XML_TOKEN_STRING) break;
                            attrs.emplace_back(attrName, nxtoken.value);
                        }
                    }

                    if (needComma) fputs(",\n", out);
                    indent(stack.size());
                    fputs("{ \"name\": ", out); jsonEscaped(tagName, out);

                    if (!attrs.empty()) {
                        fputs(", \"attributes\": {", out);
                        for (size_t i = 0; i < attrs.size(); ++i) {
                            if (i > 0) fputs(", ", out);
                            jsonEscaped(attrs[i].first, out);
                            fputs(": ", out);
                            jsonEscaped(attrs[i].second, out);
                        }
                        fputs("}", out);
                    }
                    else {
                        fputs(", \"attributes\": {}", out);
                    }

                    if (selfClosing) {
                        fputs(" }", out);
                        needComma = true;
                    }
                    else {
                        fputs(", \"children\": [\n", out);
                        stack.push_back({ tagName });
                        needComma = false;
                    }
                }
                break;
            }

            case XML_TOKEN_TEXT:
                if (collapseWhitespace && isAll(tok.value, chrWspChars)) 
                {
                    // skip this text node
                    break;
                }

                if (!stack.empty()) {
                    if (needComma) fputs(",\n", out);
                    indent(stack.size());
                    fputs("{ \"text\": ", out); jsonEscaped(tok.value, out); fputs(" }", out);
                    needComma = true;
                }
                break;

            default:
                // ignore
                break;
            }
        }

        // Close all remaining open elements
        while (!stack.empty()) {
            fputs("\n", out);
            indent(stack.size() - 1);
            fputs("] }", out);
            stack.pop_back();
        }

        fputs("\n", out);
    }
}
