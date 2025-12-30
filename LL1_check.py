from collections import defaultdict

class LL1Analyzer:
    """Analyzer for checking if a CFG is LL(1). I was too lazy to write this myself, so I asked Grok 4.1 to write this and I simply reviewed it."""
    
    def __init__(self):
        self.productions = defaultdict(list)
        self.non_terminals = set()
        self.terminals = set()
        self.first = defaultdict(set)
        self.follow = defaultdict(set)
        self.start_symbol = None
        self.EPSILON = 'ε'
        self.EOF = '$'
   

    def add_production(self, lhs, rhs, description = ""):
        self.non_terminals.add(lhs)
        self.productions[lhs].append((rhs, description))
        if self.start_symbol is None:
            self.start_symbol = lhs
   

    def set_start_symbol(self, symbol):
        self.start_symbol = symbol
   

    def identify_terminals(self):
        all_symbols = set()
        for lhs, prod_list in self.productions.items():
            for rhs, _ in prod_list:
                for symbol in rhs:
                    if symbol != self.EPSILON:
                        all_symbols.add(symbol)
        self.terminals = all_symbols - self.non_terminals
   

    def compute_first_sets(self):
        for terminal in self.terminals:
            self.first[terminal] = {terminal}
        
        for nt in self.non_terminals:
            self.first[nt] = set()
        
        changed = True
        while changed:
            changed = False
            for lhs, prod_list in self.productions.items():
                for rhs, _ in prod_list:
                    first_of_rhs = self._first_of_sequence(rhs)
                    old_size = len(self.first[lhs])
                    self.first[lhs] |= first_of_rhs
                    if len(self.first[lhs]) > old_size:
                        changed = True
   

    def _first_of_sequence(self, sequence):
        if not sequence or sequence == [self.EPSILON]:
            return {self.EPSILON}
        
        result = set()
        all_nullable = True
        
        for symbol in sequence:
            if symbol == self.EPSILON:
                continue
            
            if symbol in self.terminals:
                result.add(symbol)
                all_nullable = False
                break
            elif symbol in self.non_terminals:
                result |= (self.first[symbol] - {self.EPSILON})
                if self.EPSILON not in self.first[symbol]:
                    all_nullable = False
                    break
            else:
                result.add(symbol)
                all_nullable = False
                break
        
        if all_nullable:
            result.add(self.EPSILON)
        
        return result
   

    def compute_follow_sets(self):
        for nt in self.non_terminals:
            self.follow[nt] = set()
        
        if self.start_symbol:
            self.follow[self.start_symbol].add(self.EOF)
        
        changed = True
        while changed:
            changed = False
            for lhs, prod_list in self.productions.items():
                for rhs, _ in prod_list:
                    for i, symbol in enumerate(rhs):
                        if symbol in self.non_terminals:
                            rest = [s for s in rhs[i + 1:] if s != self.EPSILON]
                            
                            if rest:
                                first_of_rest = self._first_of_sequence(rest)
                                old_size = len(self.follow[symbol])
                                
                                self.follow[symbol] |= (first_of_rest - {self.EPSILON})
                                
                                if self.EPSILON in first_of_rest:
                                    self.follow[symbol] |= self.follow[lhs]
                                
                                if len(self.follow[symbol]) > old_size:
                                    changed = True
                            else:
                                old_size = len(self.follow[symbol])
                                self.follow[symbol] |= self.follow[lhs]
                                if len(self.follow[symbol]) > old_size:
                                    changed = True
   

    def check_ll1(self):
        conflicts = []
        is_ll1 = True
        
        for lhs, prod_list in self.productions.items():
            if len(prod_list) < 2:
                continue
            
            for i in range(len(prod_list)):
                for j in range(i + 1, len(prod_list)):
                    alpha, desc_i = prod_list[i]
                    beta, desc_j = prod_list[j]
                    
                    first_alpha = self._first_of_sequence(alpha)
                    first_beta = self._first_of_sequence(beta)
                    
                    intersection = (first_alpha - {self.EPSILON}) & (first_beta - {self.EPSILON})
                    if intersection:
                        is_ll1 = False
                        conflict_msg = self._format_first_first_conflict(
                            lhs, alpha, beta, first_alpha, first_beta, intersection
                        )
                        conflicts.append(conflict_msg)
                    
                    if self.EPSILON in first_alpha:
                        intersection2 = (first_beta - {self.EPSILON}) & self.follow[lhs]
                        if intersection2:
                            is_ll1 = False
                            conflict_msg = self._format_first_follow_conflict(
                                lhs, alpha, beta, first_beta, self.follow[lhs], 
                                intersection2, 1
                            )
                            conflicts.append(conflict_msg)
                    
                    if self.EPSILON in first_beta:
                        intersection3 = (first_alpha - {self.EPSILON}) & self.follow[lhs]
                        if intersection3:
                            is_ll1 = False
                            conflict_msg = self._format_first_follow_conflict(lhs, beta, alpha, first_alpha, self.follow[lhs], intersection3, 2)
                            conflicts.append(conflict_msg)
        
        return is_ll1, conflicts
   

    def _format_first_first_conflict(self, lhs, alpha, beta, first_a, first_b, intersection):
        return (
            f"FIRST/FIRST Conflict in '{lhs}':\n"
            f"  Production 1: {lhs} → {self._format_rhs(alpha)}\n"
            f"  Production 2: {lhs} → {self._format_rhs(beta)}\n"
            f"  FIRST(Prod1) = {self._format_set(first_a)}\n"
            f"  FIRST(Prod2) = {self._format_set(first_b)}\n"
            f"  Overlap: {self._format_set(intersection)}\n"
            f"  Reason: Cannot determine which production to use when seeing {self._format_set(intersection)}"
        )
    

    def _format_first_follow_conflict(self, lhs, nullable_prod, other_prod, first_other, follow_lhs, intersection, nullable_prod_num):
        return (
            f"FIRST/FOLLOW Conflict in '{lhs}':\n"
            f"  Nullable production (#{nullable_prod_num}): {lhs} → {self._format_rhs(nullable_prod)}\n"
            f"  Other production: {lhs} → {self._format_rhs(other_prod)}\n"
            f"  FIRST(other) = {self._format_set(first_other)}\n"
            f"  FOLLOW({lhs}) = {self._format_set(follow_lhs)}\n"
            f"  Overlap: {self._format_set(intersection)}\n"
            f"  Reason: When seeing {self._format_set(intersection)}, can't decide between "
            f"using ε and expecting these tokens after {lhs} or parsing the other production"
        )
    

    def _format_rhs(self, rhs):
        if not rhs or rhs == [self.EPSILON]:
            return 'ε'
        return ' '.join(rhs)
    

    def _format_set(self, s):
        return '{' + ', '.join(sorted(s)) + '}'
    

    def build_parsing_table(self):
        table = defaultdict(lambda: defaultdict(list))
        
        for lhs, prod_list in self.productions.items():
            for rhs, _ in prod_list:
                first_rhs = self._first_of_sequence(rhs)
                
                for terminal in first_rhs:
                    if terminal != self.EPSILON:
                        table[lhs][terminal].append(rhs)
                
                if self.EPSILON in first_rhs:
                    for terminal in self.follow[lhs]:
                        table[lhs][terminal].append(rhs)
        
        return table
    

    def print_grammar(self):
        print("\nGRAMMAR PRODUCTIONS")
        print("=" * 70)
        for lhs in sorted(self.productions.keys()):
            for rhs, desc in self.productions[lhs]:
                rhs_str = self._format_rhs(rhs)
                if desc:
                    print(f"  {lhs} → {rhs_str}  // {desc}")
                else:
                    print(f"  {lhs} → {rhs_str}")
    

    def print_first_sets(self):
        print("\nFIRST SETS")
        print("=" * 70)
        for nt in sorted(self.non_terminals):
            print(f"  FIRST({nt}) = {self._format_set(self.first[nt])}")
    

    def print_follow_sets(self):
        print("\nFOLLOW SETS")
        print("=" * 70)
        for nt in sorted(self.non_terminals):
            print(f"  FOLLOW({nt}) = {self._format_set(self.follow[nt])}")
    

    def print_parsing_table(self, show_all=False):
        table = self.build_parsing_table()
        print("\nLL(1) PARSING TABLE" + (" (conflicts only)" if not show_all else ""))
        print("=" * 70)
        
        has_conflicts = False
        for nt in sorted(table.keys()):
            nt_has_conflict = any(len(prods) > 1 for prods in table[nt].values())
            
            if show_all or nt_has_conflict:
                print(f"\n  {nt}:")
                for terminal in sorted(table[nt].keys()):
                    prods = table[nt][terminal]
                    if len(prods) > 1:
                        has_conflicts = True
                        print(f"    [{terminal}] -> CONFLICT:")
                        for p in prods:
                            print(f"           {self._format_rhs(p)}")
                    elif show_all:
                        print(f"    [{terminal}] -> {self._format_rhs(prods[0])}")
        
        if not has_conflicts and not show_all:
            print("\n  No conflicts in parsing table.")
    

def build_grammar():
    g = LL1Analyzer()
    
    # Types
    g.add_production("<Ty>", ["int"], "Basic type")
    g.add_production("<Ty>", ["bool"], "Basic type")
    g.add_production("<Ty>", ["char"], "Basic type")
    g.add_production("<Ty>", ["uint"], "Basic type")
    g.add_production("<Ty>", ["ID"], "Basic type: user-defined")
    
    g.add_production("<VaD>", ["<Ty>", "ID"], "Variable decl")
    
    g.add_production("<VaDS_tail>", [";", "<VaD>", "<VaDS_tail>"], " ")
    g.add_production("<VaDS_tail>", ["ε"], " ")
    
    g.add_production("<VaDS>", ["<VaD>", "<VaDS_tail>"], "Var decl sequence")
    
    g.add_production("<TEprime>", ["[", "NUM", "]"], " ")
    g.add_production("<TEprime>", ["@"], " ")
    g.add_production("<TEprime>", ["ε"], " ")
    
    g.add_production("<TE>", ["<Ty>", "<TEprime>"], "Type expr")
    g.add_production("<TE>", ["struct", "{", "<VaDS>", "}"], "Struct")
    
    # Declarations
    g.add_production("<TyD>", ["typedef", "<TE>", "ID"], "Type decl")
    
    g.add_production("<TyDS_tail>", [";", "<TyD>", "<TyDS_tail>"], " ")
    g.add_production("<TyDS_tail>", ["ε"], " ")
    
    g.add_production("<TyDS>", ["<TyD>", "<TyDS_tail>"], "Type decl seq")
    
    g.add_production("<TDSO>", ["<TyDS>"], "Optional typedefs")
    g.add_production("<TDSO>", ["ε"], "No typedefs")
    
    # Lvalue
    g.add_production("<lvalue_tail>", [".", "ID", "<lvalue_tail>"], " ")
    g.add_production("<lvalue_tail>", ["[", "<Expr>", "]", "<lvalue_tail>"], " ")
    g.add_production("<lvalue_tail>", ["@", "<lvalue_tail>"], " ")
    g.add_production("<lvalue_tail>", ["&", "<lvalue_tail>"], " ")
    g.add_production("<lvalue_tail>", ["ε"], " ")
    
    g.add_production("<lvalue>", ["ID", "<lvalue_tail>"], "Lvalue")
    
    # Expressions
    g.add_production("<primary_tail>", ["(", "<PSO>", ")"], "Call")
    g.add_production("<primary_tail>", ["<lvalue_tail>"], "Postfix")
    
    g.add_production("<Primary>", ["ID", "<primary_tail>"], "ID based")
    g.add_production("<Primary>", ["-", "<Primary>"], "Negation")
    g.add_production("<Primary>", ["!", "<Primary>"], "NOT")
    g.add_production("<Primary>", ["(", "<Expr>", ")"], "Parens")
    g.add_production("<Primary>", ["<C>"], "Constant")
    g.add_production("<Primary>", ["<CC>"], "Char")
    g.add_production("<Primary>", ["<BC>"], "Bool")
    
    g.add_production("<MulExpr_tail>", ["*", "<Primary>", "<MulExpr_tail>"], " ")
    g.add_production("<MulExpr_tail>", ["/", "<Primary>", "<MulExpr_tail>"], " ")
    g.add_production("<MulExpr_tail>", ["ε"], " ")
    
    g.add_production("<MulExpr>", ["<Primary>", "<MulExpr_tail>"], " ")
    
    g.add_production("<AddExpr_tail>", ["+", "<MulExpr>", "<AddExpr_tail>"], " ")
    g.add_production("<AddExpr_tail>", ["-", "<MulExpr>", "<AddExpr_tail>"], " ")
    g.add_production("<AddExpr_tail>", ["ε"], " ")
    
    g.add_production("<AddExpr>", ["<MulExpr>", "<AddExpr_tail>"], " ")
    
    g.add_production("<RelExpr_tail>", ["<rel_op>", "<AddExpr>", "<RelExpr_tail>"], " ")
    g.add_production("<RelExpr_tail>", ["ε"], " ")
    
    g.add_production("<RelExpr>", ["<AddExpr>", "<RelExpr_tail>"], " ")
    
    g.add_production("<AndExpr_tail>", ["&&", "<RelExpr>", "<AndExpr_tail>"], " ")
    g.add_production("<AndExpr_tail>", ["ε"], " ")
    
    g.add_production("<AndExpr>", ["<RelExpr>", "<AndExpr_tail>"], " ")
    
    g.add_production("<Expr_tail>", ["||", "<AndExpr>", "<Expr_tail>"], " ")
    g.add_production("<Expr_tail>", ["ε"], " ")
    
    g.add_production("<Expr>", ["<AndExpr>", "<Expr_tail>"], " ")
    
    # Parameters
    g.add_production("<PaS_tail>", [",", "<Expr>", "<PaS_tail>"], " ")
    g.add_production("<PaS_tail>", ["ε"], " ")
    
    g.add_production("<PaS>", ["<Expr>", "<PaS_tail>"], " ")
    
    g.add_production("<PSO>", ["<PaS>"], " ")
    g.add_production("<PSO>", ["ε"], " ")
    
    # Statements
    g.add_production("<RHS>", ["<Expr>"], " ")
    g.add_production("<RHS>", ["new", "ID", "@"], "Allocation")
    
    g.add_production("<EP>", ["else", "{", "<StS>", "}"], " ")
    g.add_production("<EP>", ["ε"], " ")
    
    g.add_production("<St>", ["<lvalue>", "=", "<RHS>"], "Assignment")
    g.add_production("<St>", ["if", "<Expr>", "{", "<StS>", "}", "<EP>"], "If")
    g.add_production("<St>", ["while", "<Expr>", "{", "<StS>", "}"], "While")
    
    g.add_production("<StS_tail>", [";", "<St>", "<StS_tail>"], " ")
    g.add_production("<StS_tail>", ["ε"], " ")
    
    g.add_production("<StS>", ["<St>", "<StS_tail>"], "Stmt seq")
    
    g.add_production("<rSt>", ["return", "<Expr>"], "Return")
    
    g.add_production("<SSO>", ["<StS>"], " ")
    g.add_production("<SSO>", ["ε"], " ")
    
    # Function Body
    g.add_production("<locals>", ["local", "<VaDS>"], "Local decls")
    g.add_production("<locals>", ["ε"], "No locals")
    
    g.add_production("<body>", ["<SSO>", "<rSt>"], "Function body")
    
    # Functions
    g.add_production("<PaDS_tail>", [",", "<VaD>", "<PaDS_tail>"], " ")
    g.add_production("<PaDS_tail>", ["ε"], " ")
    
    g.add_production("<PaDS>", ["<VaD>", "<PaDS_tail>"], "Param decls")
    
    g.add_production("<PDSO>", ["<PaDS>"], " ")
    g.add_production("<PDSO>", ["ε"], " ")
    
    # Program
    g.add_production("<GDT>", [";"], "Var decl end")
    g.add_production("<GDT>", ["(", "<PDSO>", ")", "{", "<locals>", "<body>", "}"], "Function")
    
    g.add_production("<GD>", ["<Ty>", "ID", "<GDT>"], "Global decl")
    
    g.add_production("<GDs>", ["<GD>", "<GDs>"], " ")
    g.add_production("<GDs>", ["ε"], " ")
    
    g.add_production("<prog>", ["<TDSO>", "<GDs>"], "Program")
    
    g.set_start_symbol("<prog>")
    
    return g


if __name__ == "__main__":
    print("LL(1) Grammar Analyzer")
    
    analyzer = build_grammar()
    analyzer.identify_terminals()
    
    print("\nGrammar Information")
    print("=" * 70)
    print(f"Start Symbol: {analyzer.start_symbol}")
    print(f"Non-terminals ({len(analyzer.non_terminals)}):")
    for nt in sorted(analyzer.non_terminals):
        print(f"  {nt}")
    print(f"\nTerminals ({len(analyzer.terminals)}):")
    print(f"  {sorted(analyzer.terminals)}")
    
    analyzer.print_grammar()
    
    analyzer.compute_first_sets()
    analyzer.print_first_sets()
    
    analyzer.compute_follow_sets()
    analyzer.print_follow_sets()
    
    print("\nLL(1) Analysis Results")
    print("=" * 70)
    
    is_ll1, conflicts = analyzer.check_ll1()
    
    if is_ll1:
        print("\nThe grammar is LL(1).")
    else:
        print("\nThe grammar is not LL(1).")
        print(f"\nFound {len(conflicts)} conflict(s):")
        for i, conflict in enumerate(conflicts, 1):
            print(f"\nConflict #{i}")
            print(conflict)
    
    analyzer.print_parsing_table(show_all=False)
    
    print("\nSummary")
    print("=" * 70)
    print(f"Total non-terminals: {len(analyzer.non_terminals)}")
    print(f"Total terminals: {len(analyzer.terminals)}")
    print(f"Total productions: {sum(len(p) for p in analyzer.productions.values())}")
    print(f"LL(1) Status: {'PASS' if is_ll1 else 'FAIL'}")
    if not is_ll1:
        print(f"Conflicts found: {len(conflicts)}")
