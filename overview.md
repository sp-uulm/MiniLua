# Tokenizer

Der Tokenizer zerteilt einen Sourcestring in einen Vektor von `LuaToken`.

`LuaToken` ist in luatoken.h definiert und enthält ein enum der verschiedenen Tokenarten, den dafür gematchten String (also z.B. type NUMLIT und match "1.57"). Außerdem die Startposition und Länge im ursprünglichen String und die davor liegenden Whitespace Characters (ist wichtig, wenn man etwas ersetzt und dann die ursprüngliche Formatierung erhalten möchte).

Der Tokenizer selbst liegt in luaparser.h (struct `lua_tokens`). Dieser ist eigentlich nur eine Reihe regulärer Ausdrücke, die Magie macht `boost::spirit::lex::tokenize`, das aus `LuaParser::tokenize` gestartet wird.

# Parser

## LuaAST

Das Ergebnis des Parsens ist ein AST. Die Knoten des Baumes leiten jeweils von `_LuaAst` ab, bzw `LuaAst` (`using LuaAst = shared_ptr<_LuaAst>`) um nicht ständig überall shared_ptr ausschreiben zu müssen. Diese Typen sind in `luaast.h` definiert, die Typaliase in `val.h`.

## LuaParser

Die Klasse `LuaParser` (`luaparser.h`) implementiert einen recursive-descent-Parser. Jedes Nichtterminal der Grammatik hat eine entsprechende Methode, z.B. `parse_stat`, das ein Statement parst. Jede dieser Methoden erhält ein Iteratorpaar auf LuaTokens und gibt ein parse_result_t zurück. Dieses Result ist immer entweder ein shared_ptr auf eine entsprechende AST Struktur oder ein Fehlerstring.

*Im Allgemeinen wirft der Code keine Exceptions, sondern meldet Fehler immer über einen Variantentyp als Ergebnis zurück.*

Die Hilfsmethode `LuaParser::parse` stößt den Tokenizer an und beginnt dann mit `parse_chunk` mit dem Parsen.

# Interpreter

## val

Lua kennt die Typen nil, bool, number, string table und function (evtl nicht vollständig). Dies ist abgebildet durch die Klasse `val` in `val.h`, die ein Variantentyp `variant<nil, bool, double, string, cfunction_p, table_p, vallist_p, lfunction_p>` ist. Es wird versucht, die Lua Typen möglichst direkt auf C++ Typen abzubilden.

`nil` ist ein Alias für `std::monostate` bzw. 

`struct {};`, kann also keine weiteren Daten halten.

`double` wird für den Number Typ verwendet, wie auch in den meisten anderen Lua Implementierungen.

`cfunction_p` und `lfunction_p` werden für Funktionen verwendet: Im Interpreter müssen Funktionen, die in Lua implementiert sind anders behandelt werden, als (builtin) Funktionen, die in C++ geschrieben sind. Struct `cfunction` und `lfunction` sind in `val.h` deklariert.

`cfunction` hält einfach einen Funktionszeiger, dem eine `vallist` der Argumente (und optional auch der AST-Node, LuaFunctioncall) übergeben wird. Diese Funktion gibt dann ihrerseits eine Vallist der Ergebnisse oder einen Fehlerstring zurück.

*Anmerkung: Richtiger wäre es, wenn die Funktion statt einer vallist ein eval_result_t zurückgibt. Ein eval_result_t enthält zusätzlich noch eventuelle SourceChanges, sodass diese nicht als Seiteneffekt entstehen. Beispielsweise muss die bisherige Implementierung von force in gui.cpp:34 das Highlighting etc selber machen, was hässlich ist. Sinnvoller wäre es, die entstehenden SourceChanges als Ergebnis zurückzugeben und danach vom Interpreter (ASTEvaluator) gesammelt anzuwenden.*

`lfunction` benötigt zusätzlich das Environment zum Zeitpunkt der Deklaration (Closure mit statischer Bindung). Der `LuaChunk` f ist der Funktionsbody, params die formalen Parameter, an die die Argumente der Funktion vor ausführung des Bodys gebunden werden müssen.

`table` ist eine Hashmap von `val` als key und Value.

`vallist` ist ein `vector<val>` um z.B. mehrere Returnwerte einer Funktion oder Parameterpacks etc. abbilden zu können.

##ASTEvaluator

Um einen `LuaAST` zu evaluieren wird die Klasse `ASTEvaluator` aus `luainterpreter.h:47` verwendet. Sie implementiert einen Visitor aud dem AST (deshalb nutzen alle Nodeklassen des AST auch das VISITABLE Makro aus luaast.h:13).

Die visit Methoden nehmen jeweils den AST-Node, das aktuelle Environment und einen `assign_t = optional<tuple<val, bool>>` Parameter und geben ein `eval_result_t`, also einen Wert mit optionalen SourceChanges oder einen Fehlerstring zurück. Der assign Parameter dient dazu anzuzeigen, ob der Wert in einem Rechts- (true) oder Linkskontext (false) ausgewertet werden soll, also z.B: auf der rechten oder linken Seite einer Zuweisung steht und welcher Wert in diesem Fall zugewiesen werden soll. Die Makros EVAL, EVALL und EVALR helfen, Ausdrücke rekursiv zu evaluieren (mit dem entsprechenden Rechts- oder Linkskontext).

## Environment

Ein Lua-Environment (Klasse `Environment` aus `environment.h:9`) nutzt ein Table t um lokale Variablen zu speichern und wird einerseits für jeden Stackframe/Scope/Closure verwendet und für alle globalen Variablen (global Table _G).

## builtin Funktionen

Die Methode `Environment::populate_stdlib` (`environment.cpp:451`) initialisiert das Environment mit ein paar gängigen mathematischen Funktionen, print und type und kann als Vorlage verwendet werden um eigene Funktionen in Lua verfügbar zu machen.

## Operatoren

`operators.h` definiert die üblichen Operatoren auf Werten, einerseits als C++ Operatoren, sodass `val` Objekte bequem verwendet werden können, aber auch die Funktionen, die beim Evaluieren einer LuaBinop Expression in ASTEvaluator verwendet werden. Dabei wird die source Information (siehe sourceexp) beachtet und entsprechende sourcebinop Knoten erzeugt.

# Source Location Tracking

## SourceExp

Ein `val` kann eine Source-Expression enthalten (`val::source`). Diese ist ein Baum, der alle Informationen enthält, woher dieser Wert stammt und welche Berechnungen darauf ausgeführt wurden.

Eine `sourceexp` kann ein `sourceval`, ein `sourcebinop`, ein `sourceunop` oder eine andere von `sourceexp` abgeleitete Klasse sein.

Im einfachsten Fall wurde der Wert vom Parser aus einem Literal geparst. In diesem Fall entählt der Wert ein `sourceval`, bestehend aus den entsprechenden Tokens (und damit der bekannten Position im Quelltext). Wenn hier ein anderer Wert erzwungen werden soll kann `sourceval::forceValue` die nötigen Änderungen sehr leicht berechnen: die Tokens werden einfach durch den neuen Wert ersetzt.

`forceValue` gibt dabei immer einen `source_change_t = optional<shared_ptr<SourceChange>>`zurück, das dann in einem separaten Schritt auf den Quelltext angewendet werden kann.

In komplexeren Fällen kann der Wert z.B. durch Anwendung eines binären Operators entstanden sein. In diesem Fall ist die Source ein `sourcebinop` der wiederum auf die zwei Werte des linken und rechten Operanden verweist. Diese Werte haben jeweils eventuell eine Source. Soll hier mit `forceValue` ein anderer Wert gesetzt werden muss rekursiv ein Pfad gefunden werden, der zu einem `sourceval` (also einem aus einem Literal entstandenen Wert) führt und dann dieses geändert werden. Da bei jeder Expression alle Operanden bekannt sind, kann passend zurückgerechnet werden.

*Beispiel: `(5*2)+3` soll `7` ergeben. Eine Möglichkeit ist, die linke Seite anzupassen, also muss `(5*2) 7-3=4` ergeben. Wieder die linke Seite anpassen ergibt `5` muss durch `4/2=2` ersetzt werden.*

## SourceChange

Wertänderungen liefern immer ein `SourceChange` Objekt (`sourcechange.h`). Die einfachste Form ist ein `SourceAssignment`: eine Ersetzung eines `LuaToken` durch einen Replacementstring.

Ein Aufruf von `forceValue` kann die gewünschte Änderung aber häufig auf viele verschiedene Arten erreichen: z.B. im Beispiel die rechte statt der linken Seite anpassen, also 3 durch -3 ersetzen. `forceValue` gibt daher immer alle Möglichkeiten als Term von `SourceChangeOr`/`SourceChangeAnd` zurück.

Um eine dieser Möglichkeiten anzuwenden kann der `ApplySCVisitor` aus `sourcechange.h:18` verwendet werden. Dieser implementiert einen einfachen left-bias, versucht also immer die linke/erste Variante anzuwenden und sammelt einen Vektor von `SourceAssignment`s (changes). Diese können auch mit apply_changes direkt auf den Vektor von LuaTokens (aus dem Parser) angewendet werden.

Ein eigener Editor kann vom ApplySCVisitor ableiten und z.B. auswählen lassen, welche der Varianten angewendet werden soll, die Änderungen zusätzlich highlighen etc.