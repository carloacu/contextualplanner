#ifndef INCLUDE_CONTEXTUALPLANNER_CONDITION_HPP
#define INCLUDE_CONTEXTUALPLANNER_CONDITION_HPP

#include <functional>
#include <map>
#include <memory>
#include <set>
#include <vector>
#include "../util/api.hpp"
#include "factoptional.hpp"


namespace cp
{
struct WorldState;
struct ConditionNode;
struct ConditionExists;
struct ConditionNot;
struct ConditionFact;
struct ConditionNumber;
struct Entity;
struct Parameter;


/// Condition with a tree structure to check in a world.
struct CONTEXTUALPLANNER_API Condition
{
  /**
   * @brief Create a condition from a string.
   * @param[in] pStr String to convert as a condition.
   * @return New condition created.
   */
  static std::unique_ptr<Condition> fromStr(const std::string& pStr,
                                            const Ontology& pOntology,
                                            const SetOfEntities& pEntities,
                                            const std::vector<Parameter>& pParameters);

  /**
   * @brief Convert the condition to a string.
   * @param[in] pFactWriterPtr Specific function to use to convert a fact to a string.
   * @return Condition converted to a string.
   */
  virtual std::string toStr(const std::function<std::string(const Fact&)>* pFactWriterPtr = nullptr,
                            bool pPrintAnyFluent = true) const = 0;


  /// Check if this condition contains a fact or the negation of the fact.
  virtual bool hasFact(const Fact& pFact) const = 0;

  /**
   * @brief Check if this condition contains an optional fact with parameters compatibility.
   * @param[in] pFactOptional Optional fact to consider.
   * @param[in] pFactParameters Optional fact parameters to possible values. The possible value will not be considered here.
   * @param[in] pOtherFactParametersPtr Another map of fact parameters to possible values.
   * @param[in] pConditionParameters Parameters of the condition.
   * @param[in] pIsWrappingExpressionNegated Is the expression wrapping this call is negated.
   * @return True if this condition contains an optional fact with parameters compatibility.
   */
  virtual bool containsFactOpt(const FactOptional& pFactOptional,
                               const std::map<Parameter, std::set<Entity>>& pFactParameters,
                               const std::map<Parameter, std::set<Entity>>* pOtherFactParametersPtr,
                               const std::vector<Parameter>& pConditionParameters,
                               bool pIsWrappingExpressionNegated = false) const = 0;

  /**
   * @brief Iterate over all the optional facts.
   * @param[in] pFactCallback Callback called for each optional fact of the condition.
   * @param[in] pIsWrappingExpressionNegated Is the expression wrapping this call is negated.
   */
  virtual void forAll(const std::function<void (const FactOptional&, bool)>& pFactCallback,
                      bool pIsWrappingExpressionNegated = false,
                      bool pIgnoreFluent = false) const = 0;


  /**
   * @brief Find a condition fact candidate a fact that is in the effect of the preceding action.
   * @param[in] pDoesConditionFactMatchFactFromEffect Callback called for each optional fact until the callback returns true.
   * @param[in] pWorldState World state to consider.
   * @param[in] pFactFromEffect Fact that is in the effect of the preceding action.
   * @param[in] pFactFromEffectParameters pFactFromEffect parameters.
   * @param[in] pFactFromEffectTmpParametersPtr pFactFromEffect other parameters.
   * @param[in] pConditionParametersToPossibleArguments Map of the parameters of the condition to their possible arguments.
   * @param[in] pIsWrappingExpressionNegated Is the expression wrapping this call is negated.
   * @return True if one callback returned true, false otherwise.
   */
  virtual bool findConditionCandidateFromFactFromEffect(
      const std::function<bool (const FactOptional&)>& pDoesConditionFactMatchFactFromEffect,
      const WorldState& pWorldState,
      const Fact& pFactFromEffect,
      const std::map<Parameter, std::set<Entity>>& pFactFromEffectParameters,
      const std::map<Parameter, std::set<Entity>>* pFactFromEffectTmpParametersPtr,
      const std::map<Parameter, std::set<Entity>>& pConditionParametersToPossibleArguments,
      bool pIsWrappingExpressionNegated = false) const = 0;

  /**
   * @brief Iterate over all the optional facts with fact value resolution according to the world sate until the callback returns false.
   * @param[in] pFactCallback Callback called for each optional fact until the callback returns false.
   * @param[in] pWorldState World state to consider.
   * @return False if one callback returned false, true otherwise.
   */
  virtual bool untilFalse(const std::function<bool (const FactOptional&)>& pFactCallback,
                          const WorldState& pWorldState) const = 0;

  /**
   * @brief Check if this condition is true for a specific world state.
   * @param[in] pWorldState World state to consider.
   * @param[in] pPunctualFacts Punctual facts raised right now.
   * @param[in] pRemovedFacts Facts present in the world state but that should be consider as not present.
   * @param[in, out] pConditionParametersToPossibleArguments Map of the parameters of the condition to their possible arguments <br/>
   * refreshed if it is necessary to have the condition true.
   * @param[out] pCanBecomeTruePtr If this condition can become true according to the facts that can become true in the world state.
   * @param[in] pIsWrappingExpressionNegated Is the expression wrapping this call is negated.
   * @return True if this condition is satisfied.
   */
  virtual bool isTrue(const WorldState& pWorldState,
                      const std::set<Fact>& pPunctualFacts = {},
                      const std::set<Fact>& pRemovedFacts = {},
                      std::map<Parameter, std::set<Entity>>* pConditionParametersToPossibleArguments = nullptr,
                      bool* pCanBecomeTruePtr = nullptr,
                      bool pIsWrappingExpressionNegated = false) const = 0;

  /**
   * @brief Check if the condition can become true according to the facts that can become true in a world state.
   * @param[in] pWorldState World state to consider.
   * @param[in] pParameters Parameter of the condition.
   * @param[in] pIsWrappingExpressionNegated Is the expression wrapping this call is negated.
   * @return True if this condition can be satisfied.
   */
  virtual bool canBecomeTrue(const WorldState& pWorldState,
                             const std::vector<Parameter>& pParameters,
                             bool pIsWrappingExpressionNegated = false) const = 0;

  /// Equality operators.
  virtual bool operator==(const Condition& pOther) const = 0;
  virtual bool operator!=(const Condition& pOther) const { return !operator==(pOther); }

  /**
   * @brief Convert this condition to a value.
   * @param[in] pWorldState World state use to extract value of the facts.
   * @return The condition converted to a string value.
   */
  virtual std::optional<Entity> getFluent(const WorldState& pWorldState) const = 0;

  /**
   * @brief Create a copy of this condition with arguments filling (or not if pConditionParametersToArgumentPtr is nullptr).
   * @param pConditionParametersToArgumentPtr Parameters to replace by their argument in the new condition to create.
   * @param pInvert True if the cloned condition should be inverted.
   * @return A copy of this condition with arguments filling.
   */
  virtual std::unique_ptr<Condition> clone(const std::map<Parameter, Entity>* pConditionParametersToArgumentPtr = nullptr,
                                           bool pInvert = false) const = 0;

  std::set<FactOptional> getFactToIgnoreInCorrespondingEffect() const;

  /// Cast to ConditionNode* is possible.
  virtual const ConditionNode* fcNodePtr() const = 0;
  virtual ConditionNode* fcNodePtr() = 0;

  /// Cast to ConditionExists* is possible.
  virtual const ConditionExists* fcExistsPtr() const = 0;
  virtual ConditionExists* fcExistsPtr() = 0;

  /// Cast to ConditionNot* is possible.
  virtual const ConditionNot* fcNotPtr() const = 0;
  virtual ConditionNot* fcNotPtr() = 0;

  /// Cast to ConditionFact* is possible.
  virtual const ConditionFact* fcFactPtr() const = 0;
  virtual ConditionFact* fcFactPtr() = 0;

  /// Cast to ConditionNumber* is possible.
  virtual const ConditionNumber* fcNbPtr() const = 0;
  virtual ConditionNumber* fcNbPtr() = 0;
};


enum class ConditionNodeType
{
  EQUALITY,
  SUPERIOR,
  INFERIOR,
  AND,
  OR,
  PLUS,
  MINUS
};


/// Condition tree node that holds children.
struct CONTEXTUALPLANNER_API ConditionNode : public Condition
{
  std::string toStr(const std::function<std::string(const Fact&)>* pFactWriterPtr,
                    bool pPrintAnyFluent) const override;

  ConditionNode(ConditionNodeType pNodeType,
                std::unique_ptr<Condition> pLeftOperand,
                std::unique_ptr<Condition> pRightOperand);

  bool hasFact(const Fact& pFact) const override;
  bool containsFactOpt(const FactOptional& pFactOptional,
                       const std::map<Parameter, std::set<Entity>>& pFactParameters,
                       const std::map<Parameter, std::set<Entity>>* pOtherFactParametersPtr,
                       const std::vector<Parameter>& pConditionParameters,
                       bool pIsWrappingExpressionNegated) const override;
  void forAll(const std::function<void (const FactOptional&, bool)>& pFactCallback,
              bool pIsWrappingExpressionNegated,
              bool pIgnoreFluent) const override;
  bool findConditionCandidateFromFactFromEffect(const std::function<bool (const FactOptional&)>& pDoesConditionFactMatchFactFromEffect,
                                                const WorldState& pWorldState,
                                                const Fact& pFactFromEffect,
                                                const std::map<Parameter, std::set<Entity>>& pFactFromEffectParameters,
                                                const std::map<Parameter, std::set<Entity>>* pFactFromEffectTmpParametersPtr,
                                                const std::map<Parameter, std::set<Entity>>& pConditionParametersToPossibleArguments,
                                                bool pIsWrappingExpressionNegated) const override;
  bool untilFalse(const std::function<bool (const FactOptional&)>& pFactCallback,
                  const WorldState& pWorldState) const override;
  bool isTrue(const WorldState& pWorldState,
              const std::set<Fact>& pPunctualFacts,
              const std::set<Fact>& pRemovedFacts,
              std::map<Parameter, std::set<Entity>>* pConditionParametersToPossibleArguments,
              bool* pCanBecomeTruePtr,
              bool pIsWrappingExpressionNegated) const override;
  bool canBecomeTrue(const WorldState& pWorldState,
                     const std::vector<Parameter>& pParameters,
                     bool pIsWrappingExpressionNegated) const override;
  bool operator==(const Condition& pOther) const override;

  std::optional<Entity> getFluent(const WorldState& pWorldState) const override;

  std::unique_ptr<Condition> clone(const std::map<Parameter, Entity>* pConditionParametersToArgumentPtr,
                                   bool pInvert) const override;

  const ConditionNode* fcNodePtr() const override { return this; }
  ConditionNode* fcNodePtr() override { return this; }
  const ConditionExists* fcExistsPtr() const override { return nullptr; }
  ConditionExists* fcExistsPtr() override { return nullptr; }
  const ConditionNot* fcNotPtr() const override { return nullptr; }
  ConditionNot* fcNotPtr() override { return nullptr; }
  const ConditionFact* fcFactPtr() const override { return nullptr; }
  ConditionFact* fcFactPtr() override { return nullptr; }
  const ConditionNumber* fcNbPtr() const override { return nullptr; }
  ConditionNumber* fcNbPtr() override { return nullptr; }

  ConditionNodeType nodeType;
  std::unique_ptr<Condition> leftOperand;
  std::unique_ptr<Condition> rightOperand;
};


/// Condition tree to manage exists operator.
struct CONTEXTUALPLANNER_API ConditionExists : public Condition
{
  std::string toStr(const std::function<std::string(const Fact&)>* pFactWriterPtr,
                    bool pPrintAnyFluent) const override;

  ConditionExists(const Parameter& pParameter,
                  std::unique_ptr<Condition> pCondition);

  bool hasFact(const Fact& pFact) const override;
  bool containsFactOpt(const FactOptional& pFactOptional,
                       const std::map<Parameter, std::set<Entity>>& pFactParameters,
                       const std::map<Parameter, std::set<Entity>>* pOtherFactParametersPtr,
                       const std::vector<Parameter>& pConditionParameters,
                       bool pIsWrappingExpressionNegated) const override;
  void forAll(const std::function<void (const FactOptional&, bool)>& pFactCallback,
              bool pIsWrappingExpressionNegated,
              bool pIgnoreFluent) const override;

  bool findConditionCandidateFromFactFromEffect(
      const std::function<bool (const FactOptional&)>& pDoesConditionFactMatchFactFromEffect,
      const WorldState& pWorldState,
      const Fact& pFactFromEffect,
      const std::map<Parameter, std::set<Entity>>& pFactFromEffectParameters,
      const std::map<Parameter, std::set<Entity>>* pFactFromEffectTmpParametersPtr,
      const std::map<Parameter, std::set<Entity>>& pConditionParametersToPossibleArguments,
      bool pIsWrappingExpressionNegated) const override;

  bool untilFalse(const std::function<bool (const FactOptional&)>& pFactCallback,
                  const WorldState& pWorldState) const override { return true; } // TODO
  bool isTrue(const WorldState& pWorldState,
              const std::set<Fact>& pPunctualFacts,
              const std::set<Fact>& pRemovedFacts,
              std::map<Parameter, std::set<Entity>>* pConditionParametersToPossibleArguments,
              bool* pCanBecomeTruePtr,
              bool pIsWrappingExpressionNegated) const override;
  bool canBecomeTrue(const WorldState& pWorldState,
                     const std::vector<Parameter>& pParameters,
                     bool pIsWrappingExpressionNegated) const override;
  bool operator==(const Condition& pOther) const override;

  std::optional<Entity> getFluent(const WorldState&) const override { return {}; }

  std::unique_ptr<Condition> clone(const std::map<Parameter, Entity>* pConditionParametersToArgumentPtr,
                                   bool pInvert) const override;

  const ConditionNode* fcNodePtr() const override { return nullptr; }
  ConditionNode* fcNodePtr() override { return nullptr; }
  const ConditionExists* fcExistsPtr() const override { return this; }
  ConditionExists* fcExistsPtr() override { return this; }
  const ConditionNot* fcNotPtr() const override { return nullptr; }
  ConditionNot* fcNotPtr() override { return nullptr; }
  const ConditionFact* fcFactPtr() const override { return nullptr; }
  ConditionFact* fcFactPtr() override { return nullptr; }
  const ConditionNumber* fcNbPtr() const override { return nullptr; }
  ConditionNumber* fcNbPtr() override { return nullptr; }

  /// Variable to check the existance in the condition
  Parameter parameter;
  /// Expression to check
  std::unique_ptr<Condition> condition;
};


/// Condition tree node that holds the negation of a condition.
struct CONTEXTUALPLANNER_API ConditionNot : public Condition
{
  std::string toStr(const std::function<std::string(const Fact&)>* pFactWriterPtr,
                    bool pPrintAnyFluent) const override;

  ConditionNot(std::unique_ptr<Condition> pCondition);

  bool hasFact(const Fact& pFact) const override;
  bool containsFactOpt(const FactOptional& pFactOptional,
                       const std::map<Parameter, std::set<Entity>>& pFactParameters,
                       const std::map<Parameter, std::set<Entity>>* pOtherFactParametersPtr,
                       const std::vector<Parameter>& pConditionParameters,
                       bool pIsWrappingExpressionNegated) const override;
  void forAll(const std::function<void (const FactOptional&, bool)>& pFactCallback,
              bool pIsWrappingExpressionNegated,
              bool pIgnoreFluent) const override;

  bool findConditionCandidateFromFactFromEffect(
      const std::function<bool (const FactOptional&)>& pDoesConditionFactMatchFactFromEffect,
      const WorldState& pWorldState,
      const Fact& pFactFromEffect,
      const std::map<Parameter, std::set<Entity>>& pFactFromEffectParameters,
      const std::map<Parameter, std::set<Entity>>* pFactFromEffectTmpParametersPtr,
      const std::map<Parameter, std::set<Entity>>& pConditionParametersToPossibleArguments,
      bool pIsWrappingExpressionNegated) const override;

  bool untilFalse(const std::function<bool (const FactOptional&)>& pFactCallback,
                  const WorldState& pWorldState) const override { return true; } // TODO
  bool isTrue(const WorldState& pWorldState,
              const std::set<Fact>& pPunctualFacts,
              const std::set<Fact>& pRemovedFacts,
              std::map<Parameter, std::set<Entity>>* pConditionParametersToPossibleArguments,
              bool* pCanBecomeTruePtr,
              bool pIsWrappingExpressionNegated) const override;
  bool canBecomeTrue(const WorldState& pWorldState,
                     const std::vector<Parameter>& pParameters,
                     bool pIsWrappingExpressionNegated) const override;
  bool operator==(const Condition& pOther) const override;

  std::optional<Entity> getFluent(const WorldState&) const override { return {}; }

  std::unique_ptr<Condition> clone(const std::map<Parameter, Entity>* pConditionParametersToArgumentPtr,
                                   bool pInvert) const override;

  const ConditionNode* fcNodePtr() const override { return nullptr; }
  ConditionNode* fcNodePtr() override { return nullptr; }
  const ConditionExists* fcExistsPtr() const override { return nullptr; }
  ConditionExists* fcExistsPtr() override { return nullptr; }
  const ConditionNot* fcNotPtr() const override { return this; }
  ConditionNot* fcNotPtr() override { return this; }
  const ConditionFact* fcFactPtr() const override { return nullptr; }
  ConditionFact* fcFactPtr() override { return nullptr; }
  const ConditionNumber* fcNbPtr() const override { return nullptr; }
  ConditionNumber* fcNbPtr() override { return nullptr; }

  /// Negated condition
  std::unique_ptr<Condition> condition;
};


/// Condition tree node that holds only an optional fact.
struct CONTEXTUALPLANNER_API ConditionFact : public Condition
{
  std::string toStr(const std::function<std::string(const Fact&)>* pFactWriterPtr,
                    bool pPrintAnyFluent) const override { return factOptional.toStr(pFactWriterPtr, pPrintAnyFluent); }

  ConditionFact(const FactOptional& pFactOptional);

  bool hasFact(const Fact& pFact) const override;
  bool containsFactOpt(const FactOptional& pFactOptional,
                       const std::map<Parameter, std::set<Entity>>& pFactParameters,
                       const std::map<Parameter, std::set<Entity>>* pOtherFactParametersPtr,
                       const std::vector<Parameter>& pConditionParameters,
                       bool pIsWrappingExpressionNegated) const override;
  void forAll(const std::function<void (const FactOptional&, bool)>& pFactCallback,
              bool pIsWrappingExpressionNegated,
              bool pIgnoreFluent) const override;
  bool findConditionCandidateFromFactFromEffect(
      const std::function<bool (const FactOptional&)>& pDoesConditionFactMatchFactFromEffect,
      const WorldState&,
      const Fact&,
      const std::map<Parameter, std::set<Entity>>&,
      const std::map<Parameter, std::set<Entity>>*,
      const std::map<Parameter, std::set<Entity>>&,
      bool pIsWrappingExpressionNegated) const override;
  bool untilFalse(const std::function<bool (const FactOptional&)>& pFactCallback,
                  const WorldState&) const override { return pFactCallback(factOptional); }
  bool isTrue(const WorldState& pWorldState,
              const std::set<Fact>& pPunctualFacts,
              const std::set<Fact>& pRemovedFacts,
              std::map<Parameter, std::set<Entity>>* pConditionParametersToPossibleArguments,
              bool* pCanBecomeTruePtr,
              bool pIsWrappingExpressionNegated) const override;
  bool canBecomeTrue(const WorldState& pWorldState,
                     const std::vector<Parameter>& pParameters,
                     bool pIsWrappingExpressionNegated) const override;
  bool operator==(const Condition& pOther) const override;

  std::optional<Entity> getFluent(const WorldState& pWorldState) const override;

  std::unique_ptr<Condition> clone(const std::map<Parameter, Entity>* pConditionParametersToArgumentPtr,
                                   bool pInvert) const override;

  const ConditionNode* fcNodePtr() const override { return nullptr; }
  ConditionNode* fcNodePtr() override { return nullptr; }
  const ConditionExists* fcExistsPtr() const override { return nullptr; }
  ConditionExists* fcExistsPtr() override { return nullptr; }
  const ConditionNot* fcNotPtr() const override { return nullptr; }
  ConditionNot* fcNotPtr() override { return nullptr; }
  const ConditionFact* fcFactPtr() const override { return this; }
  ConditionFact* fcFactPtr() override { return this; }
  const ConditionNumber* fcNbPtr() const override { return nullptr; }
  ConditionNumber* fcNbPtr() override { return nullptr; }

  FactOptional factOptional;
};



/// Condition tree node that holds only a number.
struct CONTEXTUALPLANNER_API ConditionNumber : public Condition
{
  std::string toStr(const std::function<std::string(const Fact&)>* pFactWriterPtr,
                    bool pPrintAnyFluent) const override;

  ConditionNumber(int pNb);

  bool hasFact(const Fact&) const override  { return false; }
  bool containsFactOpt(const FactOptional&,
                       const std::map<Parameter, std::set<Entity>>&,
                       const std::map<Parameter, std::set<Entity>>*,
                       const std::vector<Parameter>&,
                       bool) const override { return false; }
  void forAll(const std::function<void (const FactOptional&, bool)>&, bool, bool) const override {}
  bool findConditionCandidateFromFactFromEffect(
      const std::function<bool (const FactOptional&)>&,
      const WorldState&,
      const Fact&,
      const std::map<Parameter, std::set<Entity>>&,
      const std::map<Parameter, std::set<Entity>>*,
      const std::map<Parameter, std::set<Entity>>&,
      bool) const override { return true; }
  bool untilFalse(const std::function<bool (const FactOptional&)>&,
                  const WorldState&) const override { return true; }
  bool isTrue(const WorldState&,
              const std::set<Fact>&,
              const std::set<Fact>&,
              std::map<Parameter, std::set<Entity>>*,
              bool*,
              bool pIsWrappingExpressionNegated) const override { return !pIsWrappingExpressionNegated; }
  bool canBecomeTrue(const WorldState&,
                     const std::vector<Parameter>&,
                     bool) const override  { return true; }
  bool operator==(const Condition& pOther) const override;

  std::optional<Entity> getFluent(const WorldState& pWorldState) const override;

  std::unique_ptr<Condition> clone(const std::map<Parameter, Entity>* pConditionParametersToArgumentPtr,
                                   bool pInvert) const override;

  const ConditionNode* fcNodePtr() const override { return nullptr; }
  ConditionNode* fcNodePtr() override { return nullptr; }
  const ConditionExists* fcExistsPtr() const override { return nullptr; }
  ConditionExists* fcExistsPtr() override { return nullptr; }
  const ConditionNot* fcNotPtr() const override { return nullptr; }
  ConditionNot* fcNotPtr() override { return nullptr; }
  const ConditionFact* fcFactPtr() const override { return nullptr; }
  ConditionFact* fcFactPtr() override { return nullptr; }
  const ConditionNumber* fcNbPtr() const override { return this; }
  ConditionNumber* fcNbPtr() override { return this; }

  int nb;
};



} // !cp


#endif // INCLUDE_CONTEXTUALPLANNER_CONDITION_HPP
