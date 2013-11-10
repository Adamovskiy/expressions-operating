#ifndef SHUNTING_YARD_H
#define SHUNTING_YARD_H

#include <map>
#include <iostream> // for diagnostics
#include <stack>
#include <string>

#include "Expression.h"

class format_error:public std::logic_error
{
public:
	format_error(const std::string & what_arg) : logic_error(what_arg) {}
};

template<typename T>
class CompletableStringMap
{
	std::map<std::string, T> m_map;
public:
	enum IsInResult {IIR_NO, IIR_MAYBE, IIR_YES};
	IsInResult isIn(std::string _key) const
	{
		if(m_map.find(_key) != m_map.end())
			return IIR_YES;
		for(std::map<std::string, T>::const_iterator iter = m_map.begin(); iter != m_map.end(); iter++)
			if(iter->first.length() > _key.length() && iter->first.substr(0, _key.length()) == _key)
				return IIR_MAYBE;
		return IIR_NO;
	}

	T Get(std::string _key) const // TODO: checking for is in
	{
		return m_map.at(_key);
	}

	void Insert(std::string _key, T _value)
	{
		m_map[_key] = _value;
	}

	void Clear()
	{
		for(std::map<std::string, T>::iterator iter = m_map.begin(); iter != m_map.end(); iter++ )
			delete iter->second;
	}

};

class ExpressionFactory
{
	struct OperationInfo
	{
	private:
		enum{m_priority = 0};
		enum{m_isLeftAssoc = 0};
		enum{m_arity = 0};

	public:
		virtual Operation* Create(Expression* arg1,Expression* arg2 = NULL) = 0;

		virtual unsigned int GetArity() const = 0;
		virtual unsigned int GetPriority() const = 0;
		virtual bool IsLeftAssoc() const = 0;

		virtual ~OperationInfo() {}
	};

	template<typename T, int priority, int isLeftAssoc, int arity> // isLeftAssoc == 1 ~ left; isLeftAssoc == 0 ~ right
	struct ConcreteOperationInfo: public OperationInfo {}; // Object can't be created, if template is not specialized for current arity.

	template<typename T, int priority, int isLeftAssoc>
	struct ConcreteOperationInfo<T, priority, isLeftAssoc, 1>: public OperationInfo
	{
	private:
		enum{m_priority = priority};
		enum{m_isLeftAssoc = isLeftAssoc};
		enum{m_arity = 1};
	public:
		virtual unsigned int GetArity() const {return m_arity;}
		virtual unsigned int GetPriority() const {return m_priority;}
		virtual bool IsLeftAssoc() const {return m_isLeftAssoc == 1;}

		virtual Operation* Create(Expression* arg1,Expression* /*arg2*/ = NULL) {return new T(arg1);}
	};

	template<typename T, int priority, int isLeftAssoc>
	struct ConcreteOperationInfo<T, priority, isLeftAssoc, 2>: public OperationInfo // TODO: Get rid of copy-paste for each arity.
	{
	private:
		enum{m_priority = priority};
		enum{m_isLeftAssoc = isLeftAssoc};
		enum{m_arity = 2};
	public:
		virtual unsigned int GetArity() const {return m_arity;}
		virtual unsigned int GetPriority() const {return m_priority;}
		virtual bool IsLeftAssoc() const {return m_isLeftAssoc == 1;}

		virtual Operation* Create(Expression* arg1,Expression* arg2 = NULL) {return new T(arg1,arg2);}
	};

	CompletableStringMap<OperationInfo*> m_operationsRegister;

	enum TokenType{TT_OPENING_BRACE, TT_CLOSING_BRACE, TT_OPERATOR, TT_VALUE, TT_NO_TOKEN};

	// Helper method, transfer one operation from yard to output, using output variables
	void YardToOutput(std::stack<std::pair<TokenType, OperationInfo*>> & yard, std::stack<Expression *> & output) throw(format_error);

	// Reads and cuts first token of _source. Unknown tokens are determined as values.
	// In case of token-value, next token will be read too
	TokenType readToken(std::string & _source, std::string & o_token, TokenType & o_nextResult, std::string & o_nextToken);

public:
	ExpressionFactory();
	~ExpressionFactory();


	template<typename T, int priority, int isLeftAssoc, int arity>
	void RegisterOperation()
	{
		ConcreteOperationInfo<T, priority, isLeftAssoc, arity> * COI = new ConcreteOperationInfo<T, priority, isLeftAssoc, arity>();
		Operation * dummy = COI->Create(NULL);
		m_operationsRegister.Insert(dummy->GetSign(), COI);
		delete dummy;
	}

	Operation* CreateOperation(OperationInfo * _info, Expression* arg1, Expression* arg2 = NULL) // TODO: Do something with hooked secons argument
	{
		return _info->Create(arg1, arg2);
	}


	static std::string PrintTT(TokenType _tt) { // diagnostic tool
		switch (_tt)
		{
		case TT_CLOSING_BRACE:
			return "TT_CLOSING_BRACE";
		case TT_NO_TOKEN:
			return "TT_NO_TOKEN";
		case TT_OPENING_BRACE:
			return "TT_OPENING_BRACE";
		case TT_OPERATOR:
			return "TT_OPERATOR";
		case TT_VALUE:
			return "TT_VALUE";
		}
	}

	void LogTokens(std::string _source, std::ostream & out) // diagnostic tool
	{
		TokenType res1, res2;
		std::string token1, token2;

		while(true)
		{
			res1 = readToken(_source, token1, res2, token2);
			out << ExpressionFactory::PrintTT(res1);
			if(res1 == TT_OPERATOR || res1 == TT_VALUE)
				out << " \"" << token1 << "\"";
			out << std::endl;
			if(res1 == TT_VALUE)
			{
				out << ExpressionFactory::PrintTT(res2);
				if(res2 == TT_OPERATOR || res2 == TT_VALUE)
					out << " \"" << token2 << "\"";
				out << std::endl;
				if(res2 == TT_NO_TOKEN)
					break;
			}
			if(res1 == TT_NO_TOKEN)
				break;
		}
	}


	Expression* buildExpression(std::string _source) throw(format_error);
};
#endif // SHUNTING_YARD_H
