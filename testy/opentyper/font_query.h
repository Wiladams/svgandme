// opentype_query.h - Query system for font metadata
#pragma once

#include "opentype_container.h"
#include <vector>
#include <functional>
#include <string>
#include <cstring>
#include <cctype>
#include <algorithm>

namespace waavs {
    namespace opentype {

        // ----------------------------------------------------------------------------
        // FontQuery - standalone filter for fonts.
        // ----------------------------------------------------------------------------

        class FontQuery {
        public:
            using Predicate = std::function<bool(const OpenTypeParser&)>;

            FontQuery() = default;
            ~FontQuery() = default;

            // ----- Fluent setters (append predicates) ------------------------------

            FontQuery& family(const char* name) {
                predicates_.emplace_back([name](const OpenTypeParser& p) {
                    const char* fam = p.getFontFamily();
                    return fam && std::strcmp(fam, name) == 0;
                    });
                return *this;
            }

            FontQuery& familyCI(const char* name) {
                std::string target(name);
                std::transform(target.begin(), target.end(), target.begin(), ::tolower);
                predicates_.emplace_back([target = std::move(target)](const OpenTypeParser& p) {
                    const char* fam = p.getFontFamily();
                    if (!fam) return false;
                    std::string famLower(fam);
                    std::transform(famLower.begin(), famLower.end(), famLower.begin(), ::tolower);
                    return famLower == target;
                    });
                return *this;
            }

            FontQuery& hasFeature(Tag tag) {
                predicates_.emplace_back([tag](const OpenTypeParser& p) {
                    return p.hasFeature(tag);
                    });
                return *this;
            }

            FontQuery& hasTable(Tag tag) {
                predicates_.emplace_back([tag](const OpenTypeParser& p) {
                    return p.hasTable(tag);
                    });
                return *this;
            }

            FontQuery& minGlyphCount(uint16_t min) {
                predicates_.emplace_back([min](const OpenTypeParser& p) {
                    return p.getGlyphCount() >= min;
                    });
                return *this;
            }

            FontQuery& versionContains(const char* substr) {
                predicates_.emplace_back([substr](const OpenTypeParser& p) {
                    const char* ver = p.getFontVersion();
                    return ver && std::strstr(ver, substr) != nullptr;
                    });
                return *this;
            }

            // ----- Custom predicate -------------------------------------------------
            // Add an arbitrary predicate; 
            // return *this for chaining.
            FontQuery& addPredicate(Predicate p) {
                predicates_.push_back(std::move(p));
                return *this;
            }

            // ----- Combine predicates ----------------------------------------------

            // Add all predicates from another query to this one (AND semantics)
            FontQuery& andQuery(const FontQuery& other) {
                for (const auto& pred : other.predicates_) {
                    predicates_.push_back(pred);
                }
                return *this;
            }

            // Static helper to combine two queries into one (AND)
            static FontQuery combine(const FontQuery& a, const FontQuery& b) {
                FontQuery result;
                result.andQuery(a);
                result.andQuery(b);
                return result;
            }

            // ----- Execution -------------------------------------------------------

            // Filter a list of fonts
            std::vector<const OpenTypeParser*> execute(
                const std::vector<const OpenTypeParser*>& fonts) const {
                std::vector<const OpenTypeParser*> result;
                if (predicates_.empty()) {
                    // No predicates: return a copy of the input
                    result = fonts;
                    return result;
                }
                for (const auto* font : fonts) {
                    bool match = true;
                    for (const auto& pred : predicates_) {
                        if (!pred(*font)) {
                            match = false;
                            break;
                        }
                    }
                    if (match) result.push_back(font);
                }
                return result;
            }

            // ----- Utility ---------------------------------------------------------

            size_t predicateCount() const { return predicates_.size(); }
            bool empty() const { return predicates_.empty(); }

        private:
            std::vector<Predicate> predicates_;
        };

    } // namespace opentype
} // namespace waavs
