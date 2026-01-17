#pragma once


#include <vector>

#include "xmltypes.h"

namespace waavs {

    enum XPathTokenKind : uint32_t
    {
        END,               // End of expression
        ROOT,              // `/` (absolute root selector)
        CHILD,             // `/` (child selector)
        DESCENDANT,        // `//` (descendant selector)
        NODE,              // Element name (e.g., `svg`, `rect`, etc.)
        ATTRIBUTE,         // `@attrName` (attribute selector)
        WILDCARD_NODE,     // `*` (matches any element)
        WILDCARD_ATTRIBUTE,// `@*` (matches any attribute)
        OPERATOR,          // `=`, `!=`, `<`, `<=`, `>`, `>=`
        VALUE,             // Quoted string values (e.g., `"SVGID_7_"`)
        PREDICATE,         // `[predicate]` (filters inside brackets)
        FUNCTION,          // XPath functions (`contains()`, `starts-with()`, `text()`, etc.)
        COMMA,             // `,` (used for multiple selectors)
        UNION,             // `|` (union operator for multiple paths)
        NAMESPACE,         // `prefix:name` (namespaced elements)
        AXIS,              // `ancestor::`, `child::`, etc.
        SELF,              // `.` (current node)
        PARENT,            // `..` (parent node)
        GROUP_START,       // `(` (function argument opening)
        GROUP_END,         // `)` (function argument closing)
        LOGICAL_AND,       // `and` (logical AND operator)
        LOGICAL_OR,        // `or` (logical OR operator)
        POSITION           // Numeric position (e.g., `[3]` selects third element)
    };




    struct XPathToken
    {
        XPathTokenKind kind;
        ByteSpan value;

        XPathToken() = default;
        constexpr XPathToken(const XPathToken&) noexcept = default;
        constexpr XPathToken(XPathToken&&) noexcept = default;
        constexpr XPathToken(XPathTokenKind k, ByteSpan v) : kind(k), value(v) {}

        // Allow copy and move operations
        XPathToken& operator=(const XPathToken&) noexcept = default;
        XPathToken& operator=(XPathToken&&) noexcept = default;

    };


    struct XPathTokenizer
    {
        ByteSpan fSource;

        explicit XPathTokenizer(const ByteSpan& expr) : fSource(expr) {}

        bool next(XPathToken& token)
        {
            fSource = chunk_ltrim(fSource, chrWspChars); // Skip leading whitespace
            if (!fSource)
                return false;

            if (*fSource == '/')
            {
                if (fSource.size() > 1 && fSource[1] == '/')
                {
                    token = XPathToken(XPathTokenKind::DESCENDANT, fSource.take(2));
                    fSource += 2;
                }
                else
                {
                    token = XPathToken(XPathTokenKind::CHILD, fSource.take(1));
                    fSource++;
                }
            }
            else if (*fSource == '.')
            {
                if (fSource.size() > 1 && fSource[1] == '.')
                {
                    token = XPathToken(XPathTokenKind::PARENT, fSource.take(2));
                    fSource += 2;
                }
                else
                {
                    token = XPathToken(XPathTokenKind::SELF, fSource.take(1));
                    fSource++;
                }
            }
            else if (*fSource == '*')
            {
                token = XPathToken(XPathTokenKind::WILDCARD_NODE, fSource.take(1));
                fSource++;
            }
            else if (*fSource == '@')
            {
                fSource++;
                if (*fSource == '*')
                {
                    token = XPathToken(XPathTokenKind::WILDCARD_ATTRIBUTE, fSource.take(1));
                    fSource++;
                }
                else
                {
                    ByteSpan attrName = chunk_token(fSource, charset("=><![]"));
                    token = XPathToken(XPathTokenKind::ATTRIBUTE, attrName);
                }
            }
            else if (*fSource == '[')
            {
                ByteSpan predicate = chunk_read_bracketed(fSource, '[', ']');
                token = XPathToken(XPathTokenKind::PREDICATE, predicate);
            }
            else if (*fSource == '(')
            {
                token = XPathToken(XPathTokenKind::GROUP_START, fSource.take(1));
                fSource++;
            }
            else if (*fSource == ')')
            {
                token = XPathToken(XPathTokenKind::GROUP_END, fSource.take(1));
                fSource++;
            }
            else if (chrAlphaChars[*fSource])
            {
                ByteSpan identifier = chunk_token(fSource, charset("(/[]@=><!"));
                if (fSource.size() > 1 && fSource[0] == ':' && fSource[1] == ':')
                {
                    fSource += 2;
                    token = XPathToken(XPathTokenKind::AXIS, identifier);
                }
                else if (*fSource == '(')
                {
                    token = XPathToken(XPathTokenKind::FUNCTION, identifier);
                }
                else
                {
                    token = XPathToken(XPathTokenKind::NODE, identifier);
                }
            }
            else
            {
                return false;
            }

            return true;
        }
    };




}

namespace waavs {
    struct XPathExpression
    {
        struct Step
        {
            uint32_t axis = static_cast<uint32_t>(XPathTokenKind::CHILD);
            ByteSpan nodeName{};
            ByteSpan attribute{};
            ByteSpan value{};
            XPathTokenKind operatorType = XPathTokenKind::END;
        };

        std::vector<Step> steps;

		XPathExpression() = default;
        explicit XPathExpression(const ByteSpan& expr)
        {
            parse(expr);
        }

        bool parse(const ByteSpan& expr)
        {
            XPathTokenizer tokenizer(expr);
            XPathToken token;
            Step currentStep;

            while (tokenizer.next(token))
            {
                switch (token.kind)
                {
                case XPathTokenKind::ROOT:
                case XPathTokenKind::CHILD:
                case XPathTokenKind::DESCENDANT:
                    if (!currentStep.nodeName.empty())
                        steps.push_back(currentStep);
                    currentStep = Step{ token.kind, {}, {}, {}, XPathTokenKind::END };
                    break;

                case XPathTokenKind::NODE:
                    currentStep.nodeName = token.value;
                    break;

                case XPathTokenKind::ATTRIBUTE:
                    currentStep.attribute = token.value;
                    break;

                case XPathTokenKind::OPERATOR:
                    currentStep.operatorType = token.kind;
                    break;

                case XPathTokenKind::VALUE:
                    currentStep.value = token.value;
                    break;

                case XPathTokenKind::PREDICATE:
                    // Handle attribute conditionals
                    break;

                default:
                    return false;
                }
            }

            if (!currentStep.nodeName.empty())
                steps.push_back(currentStep);

            return true;
        }
    };



}

namespace waavs {
    struct XPathPredicate
    {
        XPathExpression expr; // Parsed XPath query
        const XmlElement* root = nullptr; // Stores root element
        std::vector<const XmlElement*> ancestors; // Tracks parents

        // Constructor that accepts a pre-parsed expression
        explicit XPathPredicate(const XPathExpression& expression) : expr(expression) {}

        // Constructor that takes a raw ByteSpan XPath string and parses it
        explicit XPathPredicate(const ByteSpan& xpathString)
        {
            if (!expr.parse(xpathString))
            {
                expr.steps.clear(); // Reset if parsing fails
            }
        }

        void setRoot(const XmlElement* rootElement)
        {
            root = rootElement;
            ancestors.clear();
        }

        bool operator()(const XmlElement& element)
        {
            if (expr.steps.empty())
                return false; // No valid steps means nothing matches

            // Establish the hierarchy tracking
            if (!root)
                setRoot(&element);

            const XmlElement* current = &element;
            size_t stepIndex = expr.steps.size() - 1; // Start from the last step

            while (current)
            {
                const XPathExpression::Step& step = expr.steps[stepIndex];

                switch (step.axis)
                {
                case XPathTokenKind::ROOT:
                    if (current != root)
                        return false;
                    break;

                case XPathTokenKind::CHILD:
                    if (!step.nodeName.empty() && current->name() != step.nodeName)
                        return false;
                    break;

                case XPathTokenKind::DESCENDANT:
                {
                    // Scan up the ancestor stack to find a matching parent/ancestor
                    bool found = false;
                    for (const XmlElement* ancestor : ancestors)
                    {
                        if (ancestor->name() == step.nodeName)
                        {
                            found = true;
                            break;
                        }
                    }
                    if (!found) return false;
                }
                break;

                case XPathTokenKind::WILDCARD_NODE:
                    // Any node matches, continue traversal
                    break;

                case XPathTokenKind::ATTRIBUTE:
                {
                    ByteSpan attrValue;
                    if (!current->getRawAttributeValue(step.attribute, attrValue))
                        return false; // Attribute missing

                    if (!step.value.empty() && attrValue != step.value)
                        return false; // Attribute value mismatch
                }
                break;

                case XPathTokenKind::PREDICATE:
                {
                    // Evaluate predicate conditions (e.g., `[id="value"]`)
                    ByteSpan attrValue;
                    if (!current->getRawAttributeValue(step.attribute, attrValue))
                        return false; // Attribute missing

                    if (!step.value.empty() && attrValue != step.value)
                        return false; // Attribute value mismatch
                }
                break;

                default:
                    return false; // Unsupported step
                }

                // Move up the hierarchy stack
                ancestors.push_back(current);
                current = (ancestors.size() > 1) ? ancestors[ancestors.size() - 2] : nullptr;

                // Move to the previous step
                if (stepIndex > 0)
                    stepIndex--;
                else
                    break;
            }

            return stepIndex == 0;
        }
    };




}

