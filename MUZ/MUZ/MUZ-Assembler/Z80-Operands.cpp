//
//  Z80-Operands.cpp
//  MUZ-Workshop
//
//  Created by Francis Pierot on 23/12/2018.
//  Copyright © 2018 Francis Pierot. All rights reserved.
//

#include "Z80-Operands.h"
#include "ParseToken.h"
#include "StrUtils.h"
#include "Expression.h"
#include "CodeLine.h"
#include "Assembler.h"
#include "All-Directives.h"

#include <string>
#include <unordered_map>
#include <map>

namespace MUZ {
	
	namespace Z80 {
	
		//MARK: - Maps for register names and sub codes
		
		// Maps of names
		std::unordered_map<std::string, OperandType> registers8;    // list of acceptable 8-bit registers
		std::unordered_map<std::string, OperandType> registers16;	// list of acceptable 16-bit registers
		std::unordered_map<std::string, OperandType> conditions;	// list of acceptable conditions
		
		// Maps of sub-encoding for addressing
		std::map<OperandType, int> regsubcode;
		std::map<OperandType, int> regprefix;

		// Initialize registers map
		void InitRegisterMap() {
			
			// offsets for registers in some encodings
			regsubcode[regB] = 0;
			regsubcode[regC] = 1;
			regsubcode[regD] = 2;
			regsubcode[regE] = 3;
			regsubcode[regH] = 4;
			regsubcode[regL] = 5;
			regsubcode[regF] = 6;
			regsubcode[indHL] = 6;
			regsubcode[regA] = 7;
			//undocumented
			regsubcode[regIXH] = 4;
			regsubcode[regIXL] = 5;
			regsubcode[regIYH] = 4;
			regsubcode[regIYL] = 5;
			regprefix[regIXH] = 0xDD;
			regprefix[regIYH] = 0xFD;
			regprefix[regIXL] = 0xDD;
			regprefix[regIYL] = 0xFD;

			regsubcode[regI] = 0x07;
			regsubcode[regR] = 0x0F;
			
			regsubcode[regBC] = 0x00;
			regsubcode[regDE] = 0x10;
			regsubcode[regHL] = 0x20;
			regsubcode[regSP] = 0x30;
			regsubcode[regAF] = 0x30; // push,pop

			regsubcode[regIX] = 0x20;//hl
			regsubcode[regIY] = 0x20;//hl

			regprefix[regIX] = 0xDD;
			regprefix[regIY] = 0xFD;
			
			regsubcode[bit0] = 0x00;
			regsubcode[bit1] = 0x08;
			regsubcode[bit2] = 0x10;
			regsubcode[bit3] = 0x18;
			regsubcode[bit4] = 0x20;
			regsubcode[bit5] = 0x28;
			regsubcode[bit6] = 0x30;
			regsubcode[bit7] = 0x38;
			
			regsubcode[condNZ] = 0x00;
			regsubcode[condZ]  = 0x08;
			regsubcode[condNC] = 0x10;
			regsubcode[condC]  = 0x18;
			regsubcode[condPO] = 0x20;
			regsubcode[condPE] = 0x28;
			regsubcode[condP]  = 0x30;
			regsubcode[condM]  = 0x38;
		
			// authorized registers
			registers8["A"] = regA;
			registers8["B"] = regB;
			registers8["C"] = regC;
			registers8["D"] = regD;
			registers8["E"] = regE;
			registers8["H"] = regH;
			registers8["L"] = regL;
			registers8["I"] = regI;
			registers8["R"] = regR;
			registers8["F"] = regF;
			// undocumented
			registers8["IXH"] = regIXH;
			registers8["IXL"] = regIXL;
			registers8["IYH"] = regIYH;
			registers8["IYL"] = regIYL;

			registers16["AF"] = regAF;
			registers16["AF'"] = regAFp;
			registers16["BC"] = regBC;
			registers16["DE"] = regDE;
			registers16["HL"] = regHL;
			registers16["SP"] = regSP;
			registers16["IX"] = regIX;
			registers16["IY"] = regIY;
			
			conditions["NC"] = condNC;
			conditions["C"] = condC;
			conditions["NZ"] = condNZ;
			conditions["Z"] = condZ;
			conditions["PE"] = condPE;
			conditions["PO"] = condPO;
			conditions["P"] = condP;
			conditions["M"] = condM;

		}
		
		// singleton to launch register maps init at run-time
		struct _auto_init_register_map {
			_auto_init_register_map() {
				InitRegisterMap();
			}
		} _runtime_auto_init_register_map;
		
		//MARK: - Low level tokens analysis for operand types
		
		/** Parses current token and return the code for a 8-bit register regA to regH, regI or regR and undocumented. */
		bool OperandTools::reg8( ExpVector* tokens, int& curtoken, OperandType& reg8)
		{
			ParseToken& token =tokens->at(curtoken);
			if (token.type != tokenTypeLETTERS) return false;
			std::string source = std::to_upper(token.source);
			if (registers8.count(source)) {
				reg8 = registers8[source];
				curtoken += 1;
				return true;
			}
			return false;
		}

		/** Parses current token and return the code for a 16-bit register regAF, regAFp regBC regDE regHL regSP IX or regIY. */
		bool OperandTools::reg16( ExpVector* tokens, int& curtoken, OperandType& reg16 )
		{
			ParseToken& token =tokens->at(curtoken);
			if (token.type != tokenTypeLETTERS) return false;
			std::string source = std::to_upper(token.source);
			if (registers16.count(source)) {
				reg16 = registers16[source];
				curtoken += 1;
				return true;
			}
			return false;
		}

		/** Parses current token and return the code for an indirect access via(C): indC. */
		bool OperandTools::indirectC( ExpVector* tokens, int& curtoken, OperandType& reg )
		{
			if (curtoken + 2 >= tokens->size() ) return false;
			ParseToken* token = &tokens->at(curtoken);
			if (token->type != tokenTypePAROPEN) return false;
			token = &tokens->at(curtoken + 1);
			if (token->type != tokenTypeLETTERS) return false;
			if (token->source != "C") return false;
			token = &tokens->at(curtoken + 2);
			if (token->type != tokenTypePARCLOSE) return false;
			reg = indC;
			curtoken += 3;
			return true;
		}

		/** Parses current token and return the code for an indirect access via (HL): indHL. */
		bool OperandTools::indirectHL( ExpVector* tokens, int& curtoken, OperandType& reg )
		{
			if (curtoken + 2 >= tokens->size() ) return false;
			ParseToken* token = &tokens->at(curtoken);
			if (token->type != tokenTypePAROPEN) return false;
			token = &tokens->at(curtoken + 1);
			if (token->type != tokenTypeLETTERS) return false;
			if (token->source != "HL") return false;
			token = &tokens->at(curtoken + 2);
			if (token->type != tokenTypePARCLOSE) return false;
			reg = indHL;
			curtoken += 3;
			return true;
		}
		
		/** Parses current token and return the code for an indirect access via (BC): indBC. */
		bool OperandTools::indirectBC( ExpVector* tokens, int& curtoken, OperandType& reg )
		{
			if (curtoken + 2 >= tokens->size() ) return false;
			ParseToken* token = &tokens->at(curtoken);
			if (token->type != tokenTypePAROPEN) return false;
			token = &tokens->at(curtoken + 1);
			if (token->type != tokenTypeLETTERS) return false;
			if (token->source != "BC") return false;
			token = &tokens->at(curtoken + 2);
			if (token->type != tokenTypePARCLOSE) return false;
			reg = indBC;
			curtoken += 3;
			return true;
		}
		
		/** Parses current token and return the code for an indirect access via (DE): indDE. */
		bool OperandTools::indirectDE( ExpVector* tokens, int& curtoken, OperandType& reg )
		{
			if (curtoken + 2 >= tokens->size() ) return false;
			ParseToken* token = &tokens->at(curtoken);
			if (token->type != tokenTypePAROPEN) return false;
			token = &tokens->at(curtoken + 1);
			if (token->type != tokenTypeLETTERS) return false;
			if (token->source != "DE") return false;
			token = &tokens->at(curtoken + 2);
			if (token->type != tokenTypePARCLOSE) return false;
			reg = indDE;
			curtoken += 3;
			return true;
		}

		/** Parses current token and return the code for an indirect access via (SP): indSP. */
		bool OperandTools::indirectSP( ExpVector* tokens, int& curtoken, OperandType& reg )
		{
			if (curtoken + 2 >= tokens->size() ) return false;
			ParseToken* token = &tokens->at(curtoken);
			if (token->type != tokenTypePAROPEN) return false;
			token = &tokens->at(curtoken + 1);
			if (token->type != tokenTypeLETTERS) return false;
			if (token->source != "SP") return false;
			token = &tokens->at(curtoken + 2);
			if (token->type != tokenTypePARCLOSE) return false;
			reg = indSP;
			curtoken += 3;
			return true;
		}

		/** Parses current token and returns the code for an indirect access via (IX+d) and (IY+d): indIX, indIY.
		 	Sends back the 'd' in parameter value.
		 	Does not change current token if returning:
		 		operrTOKENNUMBER		Not enough tokens
		 		operrMISSINGPAROPEN		Missing opening parenthesis
		 		operrREGISTERNAME		Wrong register name
		 		operrWRONGOP			Not a '+' operator
		 	Changes current token if returning:
		 		operrUNSOLVED			Unsolved symbol in 'd' expression, value is 0
		 		operrOK					value is 'd' expression result
		 */
		OperandError OperandTools::indirectX( ExpVector* tokens, int& curtoken, OperandType& regX, int& value )
		{
			if (curtoken + 4 >= tokens->size() ) return operrTOKENNUMBER;
			ParseToken* token = &tokens->at(curtoken);
			if (token->type != tokenTypePAROPEN) return operrMISSINGPAROPEN;
			int indextoken = curtoken + 1;
			if (! reg16(tokens, indextoken, regX )) return operrREGISTERNAME;
			if (regX != regIX && regX != regIY) return operrREGISTERNAME;
			token = &tokens->at(curtoken + 2);
			if (token->type != tokenTypeOP_PLUS) return operrWRONGOP;
			// find closing parenthesis
			indextoken = curtoken + 3;// skip '(' regX '+'
			int parlevel = 1;
			for ( ; indextoken < tokens->size() ; indextoken++) {
				token = &tokens->at(indextoken);
				if (token->type == tokenTypePAROPEN) {
					parlevel += 1;
				} else if (token->type == tokenTypePARCLOSE) {
					parlevel -= 1;
					if (parlevel == 0) break;
				}
			}
			// evaluate the value after "+" and before closing parenthesis
			indextoken = indextoken - 1;
			ParseToken evaluated = evalNumber.Evaluate(*tokens, curtoken + 3, indextoken);
			// skip closing parenthesis
			curtoken = indextoken + 1;
			if (evaluated.unsolved) {
				// could be pass 1, signal unsolved expression
				value = 0;
				return operrUNSOLVED;
			}
			value = evaluated.asNumber();
			return operrOK;
		}

		/** Parses current token and returns the code for a bit nunmber: bit0 to bit7.
		 	Changes current token if returning:
		 		operrUNSOLVED			Unsolved symbol in 'd' expression, value is 0
		 		operrOK					value is 'd' expression result
		 	Doesn't update current token if returning:
		 		operrNOTBIT				Number is too big for a bit number, or not a number
		 */
		OperandError OperandTools::bitnumber( ExpVector* tokens, int& curtoken, OperandType& bit )
		{
			int lasttoken = -1;
			ParseToken evaluated = evalNumber.Evaluate(*tokens, curtoken, lasttoken);
			if (evaluated.unsolved) {
				bit = bit0;
				curtoken = lasttoken ;
				return operrUNSOLVED;
			}
			if ((evaluated.type == tokenTypeSTRING) || (evaluated.type == tokenTypeDECNUMBER)) {
				int value = evaluated.asNumber();
				if (value < 0 || value > 7) return operrNOTBIT;
				if (value == 0) bit = bit0;
				else if (value == 1) bit = bit1;
				else if (value == 2) bit = bit2;
				else if (value == 3) bit = bit3;
				else if (value == 4) bit = bit4;
				else if (value == 5) bit = bit5;
				else if (value == 6) bit = bit6;
				else bit = bit7;
				curtoken = lasttoken ;
				return operrOK;
			}
			return operrNOTBIT;
		}

		/** Parses current token and return the code for a condition name: condNZ to condM. */
		OperandError OperandTools::condition( ExpVector* tokens, int& curtoken, OperandType& cond )
		{
			ParseToken& token =tokens->at(curtoken);
			if (token.type != tokenTypeLETTERS) return operrNOTSTRING;
			if (conditions.count(token.source)) {
				cond = conditions[token.source];
				curtoken += 1;
				return operrOK;
			}
			return operrNOTCONDITION;
		}

		/** Parses current token and return the value for an 8-bit number.
		 Changes current token if returning:
			 operrUNSOLVED			Unsolved symbol in expression, value is 0
			 operrOK				value is expression result
		 Doesn't update current token if returning:
			 operrTOOBIG			Number is too big for a 8-bit number
			 operrNOTNUMBER			Not a number
		 */
		OperandError OperandTools::number8( ExpVector* tokens, int& curtoken, int& value )
		{
			int lasttoken = -1;
			ParseToken evaluated = evalNumber.Evaluate(*tokens, curtoken, lasttoken);
			if (evaluated.unsolved) {
				value = 0;
				curtoken = lasttoken + 1;
				return operrUNSOLVED;
			}
			if ((evaluated.type == tokenTypeSTRING) || (evaluated.type == tokenTypeDECNUMBER)) {
				value = evaluated.asNumber();
				if (value > 255) return operrTOOBIG;
				curtoken = lasttoken + 1;
				return operrOK;
			}
			return operrNOTNUMBER;
		}

		/** Parses current token and return the value for a 16-bit number. */
		OperandError OperandTools::number16( ExpVector* tokens, int& curtoken, int& value )
		{
			int lasttoken = -1;
			ParseToken evaluated = evalNumber.Evaluate(*tokens, curtoken, lasttoken);
			if (evaluated.unsolved) {
				value = 0;
				curtoken = lasttoken + 1;
				return operrUNSOLVED;
			}
			if ((evaluated.type == tokenTypeSTRING) || (evaluated.type == tokenTypeDECNUMBER)) {
				value = evaluated.asNumber();
				if (value > 65535) return operrTOOBIG;
				curtoken = lasttoken + 1;
				return operrOK;
			}
			return operrNOTNUMBER;
		}

		/** Compute a 16-bit value from a numeric expression between parenthesis. If parenthesis or a value cannot be found,
		 returns an error code. The last used token index is returned even if the expression doesn't compute a number but
		 have correct parenthesis. */
		OperandError OperandTools::indirect16( ExpVector* tokens, int curtoken, int& value, int& lasttoken )
		{
			if (curtoken + 2 >= tokens->size() ) return operrTOKENNUMBER;
			ParseToken* token = &tokens->at(curtoken);
			if (token->type != tokenTypePAROPEN) return operrMISSINGPAROPEN;
			token = &tokens->at(curtoken + 1);
			// find closing parenthesis
			lasttoken = curtoken + 2;
			int parlevel = 1;
			for ( ; lasttoken < tokens->size() ; lasttoken++) {
				token = &tokens->at(lasttoken);
				if (token->type == tokenTypePAROPEN) {
					parlevel += 1;
				} else if (token->type == tokenTypePARCLOSE) {
					parlevel -= 1;
					if (parlevel == 0) break;
				}
			}
			if (token->type != tokenTypePARCLOSE) return operrMISSINGPARCLOSE;
			// evaluate the tokens between parenthesis
			lasttoken = lasttoken - 1; // back from parenthesis close
			ParseToken evaluated = evalNumber.Evaluate(*tokens, curtoken + 1, lasttoken );
			lasttoken = lasttoken + 1;// skips  closing parenthesis
			if (evaluated.unsolved) {
				value = 0;
				return operrUNSOLVED;
			}
			if ((evaluated.type == tokenTypeSTRING) || (evaluated.type == tokenTypeDECNUMBER)) {
				value = evaluated.asNumber();
				return operrOK;
			}
			return operrNOTNUMBER;
		}
		
		//MARK: - High level functions for CodeLine operands analysis

		OperandTools::OperandTools()
		{
			evalString.SetDefaultConversion(tokenTypeSTRING);
//			evalString.SetConversion(tokenTypeLETTERS, tokenTypeSTRING);
//			evalString.SetConversion(tokenTypeDECNUMBER, tokenTypeSTRING);
//			evalString.SetConversion(tokenTypeBINNUMBER, tokenTypeSTRING);
//			evalString.SetConversion(tokenTypeOCTNUMBER, tokenTypeSTRING);
//			evalString.SetConversion(tokenTypeHEXNUMBER, tokenTypeSTRING);
			
			evalBool.SetDefaultConversion(tokenTypeBOOL);
//			evalBool.SetConversion(tokenTypeLETTERS, tokenTypeBOOL);
//			evalBool.SetConversion(tokenTypeDECNUMBER, tokenTypeBOOL);
//			evalBool.SetConversion(tokenTypeBINNUMBER, tokenTypeBOOL);
//			evalBool.SetConversion(tokenTypeOCTNUMBER, tokenTypeBOOL);
//			evalBool.SetConversion(tokenTypeHEXNUMBER, tokenTypeBOOL);
			
			evalNumber.SetDefaultConversion(tokenTypeDECNUMBER);
//			evalNumber.SetConversion(tokenTypeLETTERS, tokenTypeDECNUMBER);
//			evalNumber.SetConversion(tokenTypeDECNUMBER, tokenTypeDECNUMBER);
//			evalNumber.SetConversion(tokenTypeBINNUMBER, tokenTypeDECNUMBER);
//			evalNumber.SetConversion(tokenTypeOCTNUMBER, tokenTypeDECNUMBER);
//			evalNumber.SetConversion(tokenTypeHEXNUMBER, tokenTypeDECNUMBER);
		}
		
		OperandTools::~OperandTools()
		{
		}
		
		// helpers for instruction assembling
		bool OperandTools::RegAccept(int flags, OperandType reg)
		{
			int f = 1 << (int)reg;
			return ((f & flags) == f);
		}
		
		/** Returns the subcode for a register code. Used for instructions accepting a reg8 spec or a reg16 spec.
		 Returns a 0 for any invalid register or addressing code.
		 */
		int OperandTools::GetSubCode( OperandType reg )
		{
			if (regsubcode.count(reg))
				return regsubcode[reg];
			return 0;
		}
		/** Returns the prefix for a register code. Used for instructions accepting IX, IY and undocumented forms.
		 Returns a 0 for any invalid register or addressing code.
		 */
		int OperandTools::GetPrefix( OperandType reg )
		{
			if (regprefix.count(reg))
				return regprefix[reg];
			return 0;
		}
		
		/** Returns true if current token is recognized as an 8-bit register, and go next token. */
		OperandError OperandTools::GetReg8(CodeLine& codeline, OperandType& reg, unsigned int regs ) {
			if (!EnoughTokensLeft(codeline, 1)) return operrTOKENNUMBER;
			int worktoken = codeline.curtoken;
			if (reg8(&codeline.tokens, worktoken, reg)) {
				if (RegAccept(regs, reg)) {
					codeline.curtoken = worktoken;
					return operrOK;
				}
				return operrWRONGREGISTER;
			}
			return operrNOTREGISTER;
		}
		/** Returns true if current token is recognized as an 16-bit register, and go next token. */
		OperandError OperandTools::GetReg16(CodeLine& codeline, OperandType& reg, unsigned int regs  ) {
			if (!EnoughTokensLeft(codeline, 1)) return operrTOKENNUMBER;
			int worktoken = codeline.curtoken;
			if (reg16(&codeline.tokens, worktoken, reg)) {
				if (RegAccept(regs, reg)) {
					codeline.curtoken = worktoken;
					return operrOK;
				}
				return operrWRONGREGISTER;
			}
			return operrNOTREGISTER;
		}
		/** Returns true if current token is recognized as (C), and go next token. */
		OperandError OperandTools::GetIndC( CodeLine& codeline ) {
			if (!EnoughTokensLeft(codeline, 3)) return operrTOKENNUMBER;
			OperandType regC;
			if (indirectC(&codeline.tokens, codeline.curtoken, regC)) {
				return operrOK;
			}
			return operrWRONGREGISTER;
		}
		/** Returns true if current token is recognized as (HL), and go next token. */
		OperandError OperandTools::GetIndHL( CodeLine& codeline ) {
			if (!EnoughTokensLeft(codeline, 3)) return operrTOKENNUMBER;
			OperandType regHL;
			if (indirectHL(&codeline.tokens, codeline.curtoken, regHL)) {
				return operrOK;
			}
			return operrWRONGREGISTER;
		}
		/** Returns true if current token is recognized as (HL), and go next token. */
		OperandError OperandTools::GetIndBC( CodeLine& codeline ) {
			if (!EnoughTokensLeft(codeline, 3)) return operrTOKENNUMBER;
			OperandType regBC;
			if (indirectBC(&codeline.tokens, codeline.curtoken, regBC)) {
				return operrOK;
			}
			return operrWRONGREGISTER;
		}
		/** Returns true if current token is recognized as (HL), and go next token. */
		OperandError OperandTools::GetIndDE( CodeLine& codeline ) {
			if (!EnoughTokensLeft(codeline, 3)) return operrTOKENNUMBER;
			OperandType regDE;
			if (indirectDE(&codeline.tokens, codeline.curtoken, regDE)) {
				return operrOK;
			}
			return operrWRONGREGISTER;
		}
		/** Returns true if current token is recognized as (SP), and go next token. */
		OperandError OperandTools::GetIndSP( CodeLine& codeline ) {
			if (!EnoughTokensLeft(codeline, 3)) return operrTOKENNUMBER;
			OperandType regSP;
			if (indirectSP(&codeline.tokens, codeline.curtoken, regSP)) {
				return operrOK;
			}
			return operrWRONGREGISTER;
		}
		
		/** Returns true if current token is recognized as (IX+d) or (IY+d), and go next token. */
		OperandError OperandTools::GetIndX(CodeLine& codeline, OperandType& regX, int& value ) {
			if (!EnoughTokensLeft(codeline,5)) return operrTOKENNUMBER;
			OperandError operr = indirectX(&codeline.tokens, codeline.curtoken, regX, value);
			if (operr == operrOK) {
				return operrOK;
			}
			if (codeline.as->IsFirstPass() && (operr == operrUNSOLVED)) {
				// probably unresolved label, simulate success with neutral value
				value = 0;
				return operrOK;
			}
			return operrWRONGREGISTER;
		}
		
		/** Returns true if current token is recognized as a bit number (0-7), and go next token. */
		OperandError OperandTools::GetBitNumber(CodeLine& codeline, OperandType& bit ) {
			if (!EnoughTokensLeft(codeline,1)) return operrTOKENNUMBER;
			int worktoken = codeline.curtoken;
			if (reg8(&codeline.tokens, worktoken, bit)) return operrWRONGREGISTER;//TODO: return explicit error (register name for 8-bit value)
			if (reg16(&codeline.tokens, worktoken, bit)) return operrWRONGREGISTER;//TODO: return explicit error (register name for 8-bit value)
			OperandError operr = bitnumber(&codeline.tokens, codeline.curtoken, bit);
			if (operr == operrOK) {
				return operrOK;
			}
			if (codeline.as->IsFirstPass() && operr == operrUNSOLVED) {
				// not number: probably unresolved label, simulate success with neutral value
				bit = bit0;
				return operrOK;
			}
			return operr;
		}
		
		/** Returns true if current token is recognized as a condition, and go next token. */
		OperandError OperandTools::GetCond( CodeLine& codeline, OperandType& cond ) {
			if (!EnoughTokensLeft(codeline, 1)) return operrTOKENNUMBER;
			if (condition(&codeline.tokens, codeline.curtoken, cond) == operrOK) {
				return operrOK;
			}
			return operrNOTCONDITION;
		}
		
		/** Tests if current token is recognized as an 8-bit number expression, and go next token.
		 In case a number could not be found, the call will return:
		 - operrTOKENNUMBER if there are not enough tokens left
		 - operrWRONGREGISTER if a register name has been found
		 - operrNOTNUMBER if the expression is not a number
		 */
		OperandError OperandTools::GetNum8(CodeLine& codeline, int& value ) {
			if (!EnoughTokensLeft(codeline,1)) return operrTOKENNUMBER;
			OperandType num8;
			// forbid register names
			int worktoken = codeline.curtoken;
			if (reg8(&codeline.tokens, worktoken, num8)) return operrWRONGREGISTER;
			if (reg16(&codeline.tokens, worktoken, num8)) return operrWRONGREGISTER;
			// now only numbers or labels
			OperandError operr = number8(&codeline.tokens, codeline.curtoken, value);
			if (operr == operrOK) {
				return operrOK;
			}
			if (codeline.as->IsFirstPass() && operr == operrUNSOLVED) {
				value = 0;
				return operrOK;
			}
			return operrNOTNUMBER;
		}
		
		/** Tests if current token is recognized as a 16-bit number expression, and go next token.
		 In case a number could not be found, the call will return:
		 - operrTOKENNUMBER if there are not enough tokens left
		 - operrWRONGREGISTER if a register name has been found
		 - operrNOTNUMBER if the expression is not a number
		 */
		OperandError OperandTools::GetNum16(CodeLine& codeline, int& value ) {
			if (!EnoughTokensLeft(codeline,1)) return operrTOKENNUMBER;
			OperandType num16;
			int worktoken = codeline.curtoken;
			if (reg8(&codeline.tokens, worktoken, num16)) return operrWRONGREGISTER;
			if (reg16(&codeline.tokens, worktoken, num16)) return operrWRONGREGISTER;
			OperandError operr = number16(&codeline.tokens, codeline.curtoken, value);
			if (operr == operrOK) {
				return operrOK;
			}
			if (codeline.as->IsFirstPass() && operr == operrUNSOLVED) {
				value = 0;
				return operrOK;
			}
			return operrNOTNUMBER;
		}
		
		/** Returns true if current token is recognized as an (16-bit) indirect addressing, and go next token.
		 In case a number could not be found, the call will return:
		 - operrTOKENNUMBER if there are not enough tokens left
		 - operrWRONGREGISTER if a register name has been found
		 - operrNOTNUMBER if the expression is not a number
		 */
		OperandError OperandTools::GetInd16(CodeLine& codeline, int& value ) {
			if (!EnoughTokensLeft(codeline,3)) return operrTOKENNUMBER;
			int lasttoken;
			OperandError operr = indirect16(&codeline.tokens, codeline.curtoken, value, lasttoken);
			if (operr==operrOK) {
				codeline.curtoken = lasttoken;
				return operrOK;
			}
			if (operr==operrUNSOLVED && codeline.as->IsFirstPass()) {
				// not number: probably unresolved label, simulate success with neutral value
				value = 0;
				codeline.curtoken = lasttoken;
				return operrOK;
			}
			// other errors do not update curtoken
			return operrNOTNUMBER;
		}

	} // namespace Z80

} // namespace MUZ
