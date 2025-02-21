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

enum class WhiteHandicapBonusRule {
    NoBonus = 0,
    BonusN = 1,
    BonusNMinus1 = 2,
};

enum class TaxRule {
    NoTax = 0,
    Seki = 1,  // Surrounded empty points in seki are not counted.
    All = 2,   // All alive groups incur up to two points in tax.
};

enum class FirstPersonPassBonusRule {
    NoBonus = 0,
    Bonus = 1,
};

struct Ruleset {
    KoRule ko_rule;
    SuicideRule suicide_rule;
    ScoringRule scoring_rule;
    WhiteHandicapBonusRule white_handicap_bonus_rule;
    TaxRule tax_rule;
    FirstPersonPassBonusRule first_person_pass_bonus_rule;
};

// Common rulesets
static const Ruleset TrompTaylorRules = {
    .ko_rule = KoRule::PositionalSuperko,
    .suicide_rule = SuicideRule::Allowed,
    .scoring_rule = ScoringRule::Area,
    .white_handicap_bonus_rule = WhiteHandicapBonusRule::NoBonus,
    .tax_rule = TaxRule::NoTax,
    .first_person_pass_bonus_rule = FirstPersonPassBonusRule::NoBonus};

static const Ruleset ChineseRules = {
    .ko_rule = KoRule::Simple,
    .suicide_rule = SuicideRule::Disallowed,
    .scoring_rule = ScoringRule::Area,
    .white_handicap_bonus_rule = WhiteHandicapBonusRule::BonusN,
    .tax_rule = TaxRule::NoTax,
    .first_person_pass_bonus_rule = FirstPersonPassBonusRule::NoBonus};

static const Ruleset JapaneseRules = {
    .ko_rule = KoRule::Simple,
    .suicide_rule = SuicideRule::Disallowed,
    .scoring_rule = ScoringRule::Territory,
    .white_handicap_bonus_rule = WhiteHandicapBonusRule::NoBonus,
    .tax_rule = TaxRule::NoTax,
    .first_person_pass_bonus_rule = FirstPersonPassBonusRule::NoBonus};

static const Ruleset AGARules = {.ko_rule = KoRule::SituationalSuperko,
                                 .suicide_rule = SuicideRule::Disallowed,
                                 .scoring_rule = ScoringRule::Area,
                                 .white_handicap_bonus_rule = WhiteHandicapBonusRule::BonusNMinus1,
                                 .tax_rule = TaxRule::NoTax,
                                 .first_person_pass_bonus_rule = FirstPersonPassBonusRule::NoBonus};

static const Ruleset NewZealandRules = {
    .ko_rule = KoRule::SituationalSuperko,
    .suicide_rule = SuicideRule::Allowed,
    .scoring_rule = ScoringRule::Area,
    .white_handicap_bonus_rule = WhiteHandicapBonusRule::NoBonus,
    .tax_rule = TaxRule::NoTax,
    .first_person_pass_bonus_rule = FirstPersonPassBonusRule::NoBonus};

}  // namespace go_data_gen