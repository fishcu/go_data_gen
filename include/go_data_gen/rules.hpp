#pragma once

namespace go_data_gen {

enum class KoRule {
    Simple = 0,
    PositionalSuperko = 1,
    SituationalSuperko = 2,
};

enum class SuicideRule {
    Allowed = 0,
    Disallowed = 1,
};

enum class ScoringRule {
    Area = 0,
    Territory = 1,
};

enum class TaxRule {
    NoTax = 0,
    Seki = 1,  // Surrounded empty points in seki are not counted.
    All = 2,   // All alive groups incur up to two points in tax.
};

enum class FirstPlayerPassBonusRule {
    NoBonus = 0,
    Bonus = 1,
};

struct Ruleset {
    KoRule ko_rule;
    SuicideRule suicide_rule;
    ScoringRule scoring_rule;
    TaxRule tax_rule;
    FirstPlayerPassBonusRule first_player_pass_bonus_rule;
};

// Common rulesets
static const Ruleset TrompTaylorRules = {
    .ko_rule = KoRule::PositionalSuperko,
    .suicide_rule = SuicideRule::Allowed,
    .scoring_rule = ScoringRule::Area,
    .tax_rule = TaxRule::NoTax,
    .first_player_pass_bonus_rule = FirstPlayerPassBonusRule::NoBonus};

static const Ruleset ChineseRules = {
    .ko_rule = KoRule::Simple,
    .suicide_rule = SuicideRule::Disallowed,
    .scoring_rule = ScoringRule::Area,
    .tax_rule = TaxRule::NoTax,
    .first_player_pass_bonus_rule = FirstPlayerPassBonusRule::NoBonus};

static const Ruleset JapaneseRules = {
    .ko_rule = KoRule::Simple,
    .suicide_rule = SuicideRule::Disallowed,
    .scoring_rule = ScoringRule::Territory,
    .tax_rule = TaxRule::NoTax,
    .first_player_pass_bonus_rule = FirstPlayerPassBonusRule::NoBonus};

static const Ruleset AGARules = {.ko_rule = KoRule::SituationalSuperko,
                                 .suicide_rule = SuicideRule::Disallowed,
                                 .scoring_rule = ScoringRule::Area,
                                 .tax_rule = TaxRule::NoTax,
                                 .first_player_pass_bonus_rule = FirstPlayerPassBonusRule::NoBonus};

static const Ruleset NewZealandRules = {
    .ko_rule = KoRule::SituationalSuperko,
    .suicide_rule = SuicideRule::Allowed,
    .scoring_rule = ScoringRule::Area,
    .tax_rule = TaxRule::NoTax,
    .first_player_pass_bonus_rule = FirstPlayerPassBonusRule::NoBonus};

}  // namespace go_data_gen