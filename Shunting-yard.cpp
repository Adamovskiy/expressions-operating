#include "Shunting-yard.h"

//Utils

inline bool isSeparator(const char & c) {return c == ' ' || c == '\n' || c == '\t';}
inline bool isNumeric(const std::string & _str)
{
	try{
		std::stod(_str);
	}catch(std::invalid_argument)
	{
		return false;
	}
	return true;
}

ExpressionFactory::ExpressionFactory() // All operations must be registred here!
{
	RegisterOperation<Sum, 4, 1, 2>();
	RegisterOperation<Increment, 1, 0, 1>();
	RegisterOperation<Sub, 4, 1, 2>();
	RegisterOperation<Mul, 3, 1, 2>();
	RegisterOperation<Div, 3, 1, 2>();
}

ExpressionFactory::~ExpressionFactory()
{
	m_operationsRegister.Clear();
}

ExpressionFactory::TokenType ExpressionFactory::readToken(
	std::string & _source,
	std::string & o_token,
	ExpressionFactory::TokenType & o_nextResult,
	std::string & o_nextToken
)
{
	std::string::const_iterator iter = _source.begin();
	do // trim going ahead spaces
	{
		if(iter == _source.end())
			return TT_NO_TOKEN;
		if(!isSeparator(*iter))
			break;
		iter++;
	}while(true);
	const char currentSign = *iter;
	if(currentSign == '(')
	{
		_source.erase(_source.begin(), iter+1);
		return TT_OPENING_BRACE;
	}
	else if(currentSign == ')')
	{
		_source.erase(_source.begin(), iter+1);
		return TT_CLOSING_BRACE;
	}
	typedef CompletableStringMap<OperationInfo*>::IsInResult Iir;
	// try to read operator
	std::string token = std::string(1,currentSign);
	do
	{
		iter++;
		const Iir result1 = m_operationsRegister.isIn(token);
		if(result1 == Iir::IIR_NO) // there is no hope to find an operator
			break;
		if(iter == _source.end()) // end of source
			if(result1 == Iir::IIR_YES)
			{
				_source.clear();
				o_token = token;
				return TT_OPERATOR;
			}
			else
				break;
		const Iir result2 = m_operationsRegister.isIn(token+*iter);
		if(result1 == Iir::IIR_YES && result2 == Iir::IIR_NO) // operator is found
		{
			_source.erase(_source.begin(), iter);
			o_token = token;
			return TT_OPERATOR;
		}
		else if(result1 == Iir::IIR_MAYBE && result2 == Iir::IIR_NO) // operator was passed
		{
			while(m_operationsRegister.isIn(token) != Iir::IIR_YES)
			{
				if(iter == _source.begin())
					break;
				token.erase(token.end()-1,token.end());
				iter--;
			}
			_source.erase(_source.begin(), iter);
			o_token = token;
			return TT_OPERATOR;
		}
		else // other cases - move on
		{
			token += *iter;
		}
	}while(true);
	// this is not an operator, therefore it is a value

	token = std::string(1, *_source.begin());
	_source.erase(_source.begin(), _source.begin() + 1);

	std::string restOfToken;
	TokenType nextResult; // unused, just for formalities
	std::string nextToken; // unused, just for formalities
	const TokenType res = readToken(_source, restOfToken, nextResult, nextToken); // Read value token recursively
	if(res != TT_VALUE) // single symbol value
	{
		o_token = token;
		o_nextResult = res;
		o_nextToken = restOfToken;
	}
	else
	{
		o_token = token + restOfToken;
		o_nextResult = nextResult; // passing next token info from the end of recursion
		o_nextToken = nextToken;
	}
	return TT_VALUE;
}

void ExpressionFactory::YardToOutput(std::stack<std::pair<TokenType, OperationInfo*>> & yard, std::stack<Expression *> & output) throw(format_error)
{
	if(yard.top().second->GetArity() > output.size())
		throw format_error("Arguments count mismatch");
	Operation* newOp;
	OperationInfo * sec = yard.top().second;
	int arr = sec->GetArity();
	if(yard.top().second->GetArity() == 1)
	{
		newOp = CreateOperation(yard.top().second, output.top());
		output.pop();
	}
	else if(yard.top().second->GetArity() == 2)
	{
		Expression * arg2 = output.top(); // as output is LIFO, the second arg
		output.pop();
		newOp = CreateOperation(yard.top().second, output.top(), arg2);
		output.pop();
	}
	yard.pop();
	output.push(newOp);
}

Expression* ExpressionFactory::buildExpression(std::string _source) throw(format_error)
{
	TokenType res1, res2;
	std::string token1, token2;
	std::stack<Expression *> output;
	typedef std::pair<TokenType, OperationInfo*> YardElem;
	std::stack<YardElem> yard;


	while(true)
	{
		res1 = readToken(_source, token1, res2, token2);
		if(res1 == TT_NO_TOKEN)
		{
			while(!yard.empty())
			{
				if(yard.top().first == TT_OPENING_BRACE)
					throw format_error("Inconsistency braces");
				YardToOutput(yard, output);
			}
			break;
		}

		else if(res1 == TT_VALUE)
		{
			if(isNumeric(token1))
				output.push(new Constant(std::stod(token1)));
			else
				output.push(new Variable(token1));

			// In case of TT_VALUE readToken reads 2 tokens!
			if(res2 == TT_NO_TOKEN)
			{
				while(!yard.empty())
				{
					if(yard.top().first == TT_OPENING_BRACE)
						throw format_error("Inconsistency braces");
					YardToOutput(yard, output);
				}
				break;
			}
			res1 = res2;
			token1 = token2;
		}
		// It can be both res1 and res2 after TT_VALUE token, so here can't be elseif
		if(res1 == TT_OPERATOR)
		{
			const OperationInfo * const oi = m_operationsRegister.Get(token1);
			while(
				!yard.empty() &&
				yard.top().first == TT_OPERATOR &&
				(
					oi->IsLeftAssoc() && oi->GetPriority() >= yard.top().second->GetPriority() ||
					!oi->IsLeftAssoc() && oi->GetPriority() > yard.top().second->GetPriority()
				)
			)
				YardToOutput(yard, output);
			yard.push(YardElem(res1, m_operationsRegister.Get(token1)));
		}

		else if(res1 == TT_OPENING_BRACE)
			yard.push(YardElem(TT_OPENING_BRACE, NULL));

		else if(res1 == TT_CLOSING_BRACE)
		{
			while(yard.top().first != TT_OPENING_BRACE)
			{
				if(yard.empty())
						throw format_error("Inconsistency braces");
				YardToOutput(yard, output);
			}
			yard.pop();
		}
	}
	return output.top();
}
