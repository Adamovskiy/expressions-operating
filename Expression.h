#ifndef EXPRESSION_H
#define EXPRESSION_H

#include <string>
#include <map>

class Expression
{
	Expression(const Expression&) {}
	Expression& operator=(const Expression&) {}
public:
	typedef std::map<std::string,double>  VarSet;

	Expression(){}
	virtual ~Expression() {}

	virtual std::string Print() const = 0; // TODO: get rid of extra braces
	virtual Expression* Diff() const = 0; // WARNING: returnable expression must be deleted manually
	virtual double Calculate(VarSet & _variables) const = 0; // All used variables must be set in VarSet
	//TODO: add simplification method(*1, +0 etc.)
	virtual void SetValue(VarSet & /*_variables*/, double /*_value*/) {} // For operators like '++', '=' etc.

	void DrawGraph( void(* _callbackDrawer)(double _x, double _y), VarSet _variables, const std::string & _varX, double _minX, double _maxX, double _step)
	{
		for(double x = _minX; x <= _maxX; x += _step)
		{
			_variables[_varX] = x;
			_callbackDrawer(x, Calculate(_variables));
		}
	}
};

class Constant: public Expression
{
	double m_value;
public:
	Constant(double _value): m_value(_value) {}

	virtual std::string Print() const
	{
		if(m_value - static_cast<int>(m_value) == 0)// Do not print fractional part if it's integer
			return std::to_string(static_cast<int>(m_value));
		else
			return std::to_string(m_value);
	}

	virtual Expression* Diff() const
	{
		return new Constant(0);
	}
	virtual double Calculate(VarSet & /*_variables*/) const
	{
		return m_value;
	}

	virtual void SetValue(VarSet & _variables, double _value) {} // TODO: notify user about bad expression
};

class Variable: public Expression
{
	 std::string m_name;
public:
	Variable(std::string _name): m_name(_name) {}

	virtual std::string Print() const
	{
		return m_name;
	}

	virtual Expression* Diff() const
	{
		return new Constant(1);
	}
	virtual double Calculate(VarSet & _variables) const
	{
		const VarSet::const_iterator value = _variables.find(m_name);
		if(value == _variables.end())
			throw std::invalid_argument("Variable " + m_name + "is not set.");
		return value->second;
	}

	virtual void SetValue(VarSet & _variables, double _value) {_variables[m_name] = _value;} 
};

class Operation: public Expression
{
public:
	virtual ~Operation() {}
	virtual std::string GetSign() const = 0; // TODO: must be static and abstract simultaneously
};

class UnaryOperation: public Operation
{
protected:
	UnaryOperation(Expression* _argument) : m_argument(_argument) {}
	Expression* m_argument;
public:
	virtual ~UnaryOperation()
	{
		delete m_argument;
	}

	virtual std::string Print() const
	{
		return GetSign() + '(' + m_argument->Print() + ')';
	}
};

class BinaryOperation: public Operation
{
protected:
	BinaryOperation(Expression* _left, Expression* _right) : m_left(_left), m_right(_right) {}
	Expression* m_left, *m_right;
public:
	virtual ~BinaryOperation()
	{
		delete m_left;
		delete m_right;
	}

	virtual std::string Print() const
	{
		return '(' + m_left->Print() + GetSign() + m_right->Print() + ')';
	}
};

class Increment: public UnaryOperation
{
public:
	Increment(Expression* _argument) : UnaryOperation(_argument) {}

	virtual std::string GetSign() const {return "++";}

	virtual Expression* Diff() const
	{
		return m_argument->Diff();
	}

	virtual double Calculate(VarSet & _variables) const
	{
		double value = m_argument->Calculate(_variables) + 1;
		m_argument->SetValue(_variables, value);
		return value;
	}
};

class Sum: public BinaryOperation
{
public:
	Sum(Expression* _left, Expression* _right) : BinaryOperation(_left, _right) {}

	virtual std::string GetSign() const {return "+";}

	virtual Expression* Diff() const
	{
		return new Sum(m_left->Diff(), m_right->Diff());
	}

	virtual double Calculate(VarSet & _variables) const
	{
		return m_left->Calculate(_variables) + m_right->Calculate(_variables);
	}
};


class Sub: public BinaryOperation
{
public:
	Sub(Expression* _left, Expression* _right) : BinaryOperation(_left, _right) {}

	virtual std::string GetSign() const {return "-";}

	virtual Expression* Diff() const
	{
		return new Sub(m_left->Diff(), m_right->Diff());
	}

	virtual double Calculate(VarSet & _variables) const
	{
		return m_left->Calculate(_variables) - m_right->Calculate(_variables);
	}
};

class Mul: public BinaryOperation
{
public:
	Mul(Expression* _left, Expression* _right) : BinaryOperation(_left, _right) {}

	virtual std::string GetSign() const {return "*";}

	virtual Expression* Diff() const
	{
		return new Sum(new Mul(m_left->Diff(), m_right), new Mul(m_left, m_right->Diff()));
	}

	virtual double Calculate(VarSet & _variables) const
	{
		return m_left->Calculate(_variables) * m_right->Calculate(_variables);
	}
};

class Div: public BinaryOperation
{
public:
	Div(Expression* _left, Expression* _right) : BinaryOperation(_left, _right) {}

	virtual std::string GetSign() const {return "/";}

	virtual Expression* Diff() const
	{
		return new Div(new Sub(new Mul(m_left->Diff(), m_right), new Mul(m_left, m_right->Diff())), new Mul(m_right, m_right));
	}

	virtual double Calculate(VarSet & _variables) const
	{
		return m_left->Calculate(_variables) / m_right->Calculate(_variables);
	}
};

#endif // EXPRESSION_H
