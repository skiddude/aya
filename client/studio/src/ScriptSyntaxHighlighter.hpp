

#pragma once

// Qt Headers
#include <QSyntaxHighlighter>

// Roblox Headers
#include "Reflection/Property.hpp"

class QTextDocument;
class QTextCharFormat;

enum RBXLuaLexState
{
    AYA_LUA_DEFAULT,
    AYA_LUA_COMMENT,
    AYA_LUA_COMMENTLINE,
    AYA_LUA_NUMBER,
    AYA_LUA_WORD,
    AYA_LUA_STRING,
    AYA_LUA_CHARACTER,
    AYA_LUA_LITERALSTRING,
    AYA_LUA_PREPROCESSOR,
    AYA_LUA_OPERATOR,
    AYA_LUA_IDENTIFIER,
    AYA_LUA_STRINGEOL
};

class ScriptSyntaxHighlighter : public QSyntaxHighlighter
{
    Q_OBJECT

public:
    ScriptSyntaxHighlighter(QTextDocument* parent = 0);

    void onPropertyChanged(const Aya::Reflection::PropertyDescriptor* pDescriptor);
    void setFont(const QFont& font);

protected:
    void highlightBlock(const QString& text);

private:
    void initData();

    void setLexState(RBXLuaLexState lexState, int currentPos);

    bool checkApplyFoldState(const QString& keyword);

    QStringList m_keywordPatterns;
    QStringList m_foldStarts;
    QStringList m_foldEnds;

    QTextCharFormat m_keywordFormat;
    QTextCharFormat m_operatorFormat;
    QTextCharFormat m_numberFormat;
    QTextCharFormat m_commentFormat;
    QTextCharFormat m_stringFormat;
    QTextCharFormat m_preprocessorFormat;
    QTextCharFormat m_defaultFormat;

    int m_startSegPos;
    int m_nestLevel;
    int m_sepCount;

    RBXLuaLexState m_lexState;
};
