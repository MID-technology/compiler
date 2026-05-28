#include "../include/semantic.h"

namespace olang {

void SemanticAnalyzer::registerBuiltins() {
    voidType_ = addClass("Void", nullptr, true);
    classClass_ = addClass("Class", nullptr, true);
    anyValueClass_ = addClass("AnyValue", classClass_, true);
    anyRefClass_ = addClass("AnyRef", classClass_, true);

    registerIntegerClass();
    registerRealClass();
    registerBooleanClass();
    registerStringClass();
    registerIOClass();

    addConstructor(integerClass_, {{"p", realClass_}});

    addMethod(integerClass_, "Plus", {{"p", realClass_}}, realClass_);
    addMethod(integerClass_, "Minus", {{"p", realClass_}}, realClass_);
    addMethod(integerClass_, "Mult", {{"p", realClass_}}, realClass_);
    addMethod(integerClass_, "Div", {{"p", realClass_}}, realClass_);

    addMethod(integerClass_, "Less", {{"p", realClass_}}, booleanClass_);
    addMethod(integerClass_, "LessEqual", {{"p", realClass_}}, booleanClass_);
    addMethod(integerClass_, "Greater", {{"p", realClass_}}, booleanClass_);
    addMethod(integerClass_, "GreaterEqual", {{"p", realClass_}}, booleanClass_);
    addMethod(integerClass_, "Equal", {{"p", realClass_}}, booleanClass_);

    arrayClass_ = addClass("Array", anyRefClass_, true);
    listClass_ = addClass("List", anyRefClass_, true);
}

void SemanticAnalyzer::registerIntegerClass() {
    integerClass_ = addClass("Integer", anyValueClass_, true);

    addConstructor(integerClass_, {});
    addConstructor(integerClass_, {{"p", integerClass_}});

    addField(integerClass_, "Min", integerClass_);
    addField(integerClass_, "Max", integerClass_);

    addMethod(integerClass_, "toReal", {}, nullptr);
    addMethod(integerClass_, "toBoolean", {}, nullptr);
    addMethod(integerClass_, "ToString", {}, nullptr);
    addMethod(integerClass_, "UnaryMinus", {}, integerClass_);

    addMethod(integerClass_, "Plus", {{"p", integerClass_}}, integerClass_);
    addMethod(integerClass_, "Minus", {{"p", integerClass_}}, integerClass_);
    addMethod(integerClass_, "Mult", {{"p", integerClass_}}, integerClass_);
    addMethod(integerClass_, "Div", {{"p", integerClass_}}, integerClass_);
    addMethod(integerClass_, "Rem", {{"p", integerClass_}}, integerClass_);

    addMethod(integerClass_, "Less", {{"p", integerClass_}}, nullptr);
    addMethod(integerClass_, "LessEqual", {{"p", integerClass_}}, nullptr);
    addMethod(integerClass_, "Greater", {{"p", integerClass_}}, nullptr);
    addMethod(integerClass_, "GreaterEqual", {{"p", integerClass_}}, nullptr);
    addMethod(integerClass_, "Equal", {{"p", integerClass_}}, nullptr);
}

void SemanticAnalyzer::registerRealClass() {
    realClass_ = addClass("Real", anyValueClass_, true);

    addConstructor(realClass_, {});
    addConstructor(realClass_, {{"p", realClass_}});
    addConstructor(realClass_, {{"p", integerClass_}});

    addField(realClass_, "Min", realClass_);
    addField(realClass_, "Max", realClass_);
    addField(realClass_, "Epsilon", realClass_);

    addMethod(realClass_, "toInteger", {}, integerClass_);
    addMethod(realClass_, "ToString", {}, nullptr);
    addMethod(realClass_, "UnaryMinus", {}, realClass_);

    addMethod(realClass_, "Plus", {{"p", realClass_}}, realClass_);
    addMethod(realClass_, "Plus", {{"p", integerClass_}}, realClass_);
    addMethod(realClass_, "Minus", {{"p", realClass_}}, realClass_);
    addMethod(realClass_, "Minus", {{"p", integerClass_}}, realClass_);
    addMethod(realClass_, "Mult", {{"p", realClass_}}, realClass_);
    addMethod(realClass_, "Mult", {{"p", integerClass_}}, realClass_);
    addMethod(realClass_, "Div", {{"p", realClass_}}, realClass_);
    addMethod(realClass_, "Div", {{"p", integerClass_}}, realClass_);
    addMethod(realClass_, "Rem", {{"p", integerClass_}}, realClass_);

    addMethod(realClass_, "Less", {{"p", realClass_}}, nullptr);
    addMethod(realClass_, "Less", {{"p", integerClass_}}, nullptr);
    addMethod(realClass_, "LessEqual", {{"p", realClass_}}, nullptr);
    addMethod(realClass_, "LessEqual", {{"p", integerClass_}}, nullptr);
    addMethod(realClass_, "Greater", {{"p", realClass_}}, nullptr);
    addMethod(realClass_, "Greater", {{"p", integerClass_}}, nullptr);
    addMethod(realClass_, "GreaterEqual", {{"p", realClass_}}, nullptr);
    addMethod(realClass_, "GreaterEqual", {{"p", integerClass_}}, nullptr);
    addMethod(realClass_, "Equal", {{"p", realClass_}}, nullptr);
    addMethod(realClass_, "Equal", {{"p", integerClass_}}, nullptr);

    auto it = integerClass_->methods.find("toReal");
    if (it != integerClass_->methods.end()) {
        for (auto& m : it->second)
            if (!m.returnType) m.returnType = realClass_;
    }
}

void SemanticAnalyzer::registerBooleanClass() {
    booleanClass_ = addClass("Boolean", anyValueClass_, true);

    addConstructor(booleanClass_, {});
    addConstructor(booleanClass_, {{"p", booleanClass_}});

    addMethod(booleanClass_, "toInteger", {}, integerClass_);
    addMethod(booleanClass_, "ToString", {}, nullptr);
    addMethod(booleanClass_, "Or", {{"p", booleanClass_}}, booleanClass_);
    addMethod(booleanClass_, "And", {{"p", booleanClass_}}, booleanClass_);
    addMethod(booleanClass_, "Xor", {{"p", booleanClass_}}, booleanClass_);
    addMethod(booleanClass_, "Not", {}, booleanClass_);

    auto patchNullReturns = [&](const ClassInfoPtr& cls,
                                 const std::string& skip1,
                                 const std::string& skip2,
                                 const std::string& skip3) {
        for (auto& [n, overloads] : cls->methods) {
            if (n == skip1 || n == skip2 || n == skip3) continue;
            for (auto& m : overloads) {
                if (!m.returnType) m.returnType = booleanClass_;
            }
        }
    };
    patchNullReturns(integerClass_, "toReal", "toBoolean", "ToString");
    patchNullReturns(realClass_, "ToString", "", "");

    auto it = integerClass_->methods.find("toBoolean");
    if (it != integerClass_->methods.end()) {
        for (auto& m : it->second)
            if (!m.returnType) m.returnType = booleanClass_;
    }
}

void SemanticAnalyzer::registerStringClass() {
    stringClass_ = addClass("String", anyRefClass_, true);

    addConstructor(stringClass_, {});
    addConstructor(stringClass_, {{"p", stringClass_}});

    addMethod(stringClass_, "Length", {}, integerClass_);
    addMethod(stringClass_, "At", {{"i", integerClass_}}, stringClass_);
    addMethod(stringClass_, "Concatenate", {{"s", stringClass_}}, stringClass_);
    addMethod(stringClass_, "Substring",
              {{"from", integerClass_}, {"to", integerClass_}}, stringClass_);
    addMethod(stringClass_, "Equal", {{"s", stringClass_}}, booleanClass_);
    addMethod(stringClass_, "ToString", {}, stringClass_);

    auto patchToString = [&](const ClassInfoPtr& cls) {
        auto it = cls->methods.find("ToString");
        if (it != cls->methods.end()) {
            for (auto& m : it->second)
                if (!m.returnType) m.returnType = stringClass_;
        }
    };
    patchToString(integerClass_);
    patchToString(realClass_);
    patchToString(booleanClass_);
}

void SemanticAnalyzer::registerIOClass() {
    ioClass_ = addClass("IO", anyRefClass_, true);

    addConstructor(ioClass_, {});

    addMethod(ioClass_, "Write", {{"s", stringClass_}}, ioClass_);
    addMethod(ioClass_, "WriteLine", {{"s", stringClass_}}, ioClass_);
    addMethod(ioClass_, "Read", {}, stringClass_);
    addMethod(ioClass_, "ReadLine", {}, stringClass_);
}

}
