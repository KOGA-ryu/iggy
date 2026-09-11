@paths 1
@subject trigonometry | Trigonometry
@chapter topic_0033 | Identities

@lesson prod03_trig_identities_r | Identities and their original domains
@template lesson.v2
@block introduction | start | 3.1 | When two expressions describe the same value
@prose An identity states that two expressions agree at every angle in their common domain. An equation instead asks for the angles where a particular equality holds. To verify an identity, transform one expression by justified rules; do not assume the equality you want to prove. A few matching numerical substitutions cannot establish an identity at every angle.
@endblock
@block definition | terms | 3.2 | Coordinates and reciprocal functions
@prose Theta is a real angle measured in radians. Write s=sin(theta) for the vertical coordinate and c=cos(theta) for the horizontal coordinate of its unit-circle point. The abbreviation E names the original expression, while D denotes the full set of angles where that original expression exists. A condition such as D: s nonzero describes that retained set. An expression's numerator is above the fraction line and its denominator is below it.
@display coordinates
s=\sin\theta,\qquad c=\cos\theta,\qquad s^2+c^2=1
@prose Tangent and cotangent are coordinate quotients; secant and cosecant are reciprocals. Each definition requires its denominator to be nonzero. A quotient of functions also requires the whole divisor to be nonzero, not merely defined.
@display definitions
\tan\theta=\frac{s}{c},\quad\sec\theta=\frac{1}{c}\quad(c\ne0),\qquad\cot\theta=\frac{c}{s},\quad\csc\theta=\frac{1}{s}\quad(s\ne0)
@prose A factor is a quantity multiplied by another quantity. Terms are added or subtracted. A conjugate changes the sign between the same two terms: 1+s and 1-s are conjugates. You may cancel a common nonzero factor, but not a term within a sum.
@endblock
@block proposition | rule | 3.3 | Four reversible algebraic moves
@prose The circle relation gives 1-s squared=c squared and 1-c squared=s squared. Dividing the circle relation by nonzero c squared gives tan squared+1=sec squared. Dividing it by nonzero s squared gives 1+cot squared=csc squared. These equations inherit the divisor's exclusions.
@display pythagorean
1-s^2=c^2,\qquad1-c^2=s^2,\qquad1+\tan^2\theta=\sec^2\theta,\qquad1+\cot^2\theta=\csc^2\theta
@prose To combine fractions, multiply numerator and denominator by each missing factor. For ordinary real numbers a,b,d,e with b and e nonzero, a/b+d/e=(ae+bd)/(be). For division, (a/b)/(d/e)=ae/(bd), requiring b,e and d all nonzero. Multiplying by a nonzero factor divided by itself preserves a value.
@prose Difference of squares follows by distribution: (1-s)(1+s)=1+s-s-s squared=1-s squared. In contrast, (1+s) squared=1+s+s+s squared=1+2s+s squared. A conjugate removes the linear terms; squaring the same binomial does not.
@display products
(1-s)(1+s)=1-s^2,\qquad(1+s)^2=1+2s+s^2
@help proof
@prose Every permitted move follows from equality-preserving real-number operations. If a multiplier M is nonzero on D, proving ME=MF for the original expression E and proposed result F proves E=F on D by division by M. When products reduce using s squared+c squared=1, the resulting polynomial identity holds for every unit-circle point. This is an exact proof, not a test at selected angles.
@endblock
@block proposition | condition | 3.4 | Restrictions survive cancellation
@prose Determine D before simplifying. Sine vanishes at integer multiples of pi; cosine vanishes at pi/2 plus integer multiples of pi. The circle relation shows that 1-c vanishes only where c=1 and s=0, and 1+c only where c=-1 and s=0. Similarly, 1-s vanishes where s=1 and c=0, while 1+s vanishes where s=-1 and c=0.
@prose Thus an exclusion already implied by another denominator need not be repeated: if s is nonzero, then both 1-c and 1+c are nonzero. If c is nonzero, then both 1-s and 1+s are nonzero. But do not impose extra exclusions on angles where the original is defined. For example, c squared/c requires c nonzero; reducing it to c does not make the original quotient defined at c=0.
@prose A simplified expression can extend beyond D. The identity as posed still retains D. Before multiplying by a conjugate or clearing denominators, check that every chosen factor is nonzero throughout D. Record these restrictions in the reached working as well as in your reasoning.
@endblock
@block example | worked | 3.5 | One complete simplification
@prose Simplify this expression to a constant times a coordinate. Determine its full original domain and prove the identity there.
@display worked_given
E=\frac{2-2s^2}{c},\qquad s=\sin\theta,\quad c=\cos\theta
@help hint
@prose Inspect the original denominator first. Factor the coefficient from the complete numerator, then replace a difference of squares using the circle relation. Cancel a factor only after showing it is nonzero.
@help answer
@display
E=2c,\qquad D:\ c\ne0
@help solution
@prose The denominator is c, so the full original domain is c nonzero. These excluded angles are pi/2 plus integer multiples of pi. No sine restriction is needed. Factor the numerator: 2-2s squared=2(1-s squared). The circle relation gives 1-s squared=c squared, so the numerator is 2c squared.
@display worked_factor
E=\frac{2(1-s^2)}{c}=\frac{2c^2}{c},\qquad D:\ c\ne0
@prose Write 2c squared as 2 times c times c. Since c is nonzero on D, cancel one c to leave 2c. The factor 2 and the other c remain. This simplification does not restore the excluded cosine zeros.
@display worked_result
E=2c,\qquad D:\ c\ne0
@prose Check directly from the original: multiplying by nonzero c gives cE=2-2s squared=2(1-s squared)=2c squared. Multiplying the proposed result by c also gives c(2c)=2c squared. Division by nonzero c proves equality at every permitted angle.
@endblock
@block example | errors | 3.6 | Value, form and domain are separate questions
@prose A true equivalent expression can miss a requested form. For example, s squared/c squared equals tan squared when c is nonzero, but it is not yet written as one squared quotient function. That is a form mismatch, not a false identity. Conversely, changing a plus sign to a minus sign while combining fractions is an algebraic error. Distinguish these explanations.
@prose Cancelling an added term instead of a common factor changes the value. Forgetting a nonzero condition may add angles where the original was undefined. Introducing a new denominator may remove originally permitted angles. A complete proof checks value, requested form and the full retained domain independently.
@endblock
@block exercise | practice | 3.7 | Twelve complete identity routes
@prose Begin with four coordinate and reciprocal simplifications. Next combine unlike denominators, distribute subtraction and rationalize with conjugates. Finish with four mixed problems that choose a useful strategy and carry it through. Each route ends with a nonzero-multiplier check against the original expression.
@endblock
@block summary | summary | 3.8 | Preserve the original question
@prose Name the original expression and determine its full domain. Select a rule that advances the requested form, perform each operation on whole terms or factors, and retain exclusions in every line. Finally compare the original and proposed result using an exact identity with legal nonzero multipliers. An answer is complete only when value, form and domain all agree.
@endblock
@practice prod03_trig_identities_q01
@practice prod03_trig_identities_q02
@practice prod03_trig_identities_q03
@practice prod03_trig_identities_q04
@practice prod03_trig_identities_q05
@practice prod03_trig_identities_q06
@practice prod03_trig_identities_q07
@practice prod03_trig_identities_q08
@practice prod03_trig_identities_q09
@practice prod03_trig_identities_q10
@practice prod03_trig_identities_q11
@practice prod03_trig_identities_q12
@end

@question prod03_trig_identities_q01 | A first coordinate quotient
@template choices.v1
@version 1
@goal Simplify E to a single coordinate factor, retain its original domain and prove the identity exactly.
@given E=\frac{\left(1-{c}^{2}\right)}{s}
@domain Theta is real and measured in radians; s=sin(theta) and c=cos(theta). E names the original expression. D is its full domain, which must be retained after every cancellation.
@read prod03_trig_identities_r
@step 10 | Which condition describes the full original domain D?
@choice 11 | s\ne0
@choice 12 | c\ne0
@choice 13 | \theta\in\mathbb{R}
@answer 11
@feedback 12 | This excludes the wrong axis points: c may be zero here, while s may not.
@feedback 13 | At theta=0, s=0 makes the original fraction undefined. Cancelling later cannot restore that angle.
@after D:s\ne0
@wrong Check the requested operation, the original expression and its retained nonzero conditions.
@why Only s is in the original denominator, so s must be nonzero. Sine vanishes at integer multiples of pi. There is no cosine denominator.
@step 20 | Use the unit-circle relation to replace only the numerator by a square.
@choice 22 | \frac{{c}^{2}}{s}
@choice 23 | \frac{\left(1+{s}^{2}\right)}{s}
@choice 21 | \frac{{s}^{2}}{s}
@answer 21
@feedback 22 | c squared equals 1-s squared, not 1-c squared. The complementary coordinate was interchanged.
@feedback 23 | The complementary square requires subtraction: 1-c squared=s squared. Adding another 1 to s squared changes the numerator.
@after E=\frac{{s}^{2}}{s},\quad D:s\ne0
@wrong Check the requested operation, the original expression and its retained nonzero conditions.
@why The circle relation is s squared+c squared=1. Subtract c squared from both sides: 1-c squared=s squared. The denominator remains s.
@step 30 | Cancel the nonzero common factor and leave one coordinate factor.
@choice 32 | c
@choice 31 | s
@choice 33 | 1
@answer 31
@feedback 32 | The cancelled quotient involves two sine factors, not two cosine factors.
@feedback 33 | One factor of s remains after cancellation; s squared divided by s is not identically 1.
@after E=s,\quad D:s\ne0
@wrong Check the requested operation, the original expression and its retained nonzero conditions.
@why On D, s is nonzero. Since s squared=s times s, dividing by s leaves s, not 1. The condition s nonzero remains part of the identity.
@step 40 | Multiply the original E by the nonzero multiplier s. Which polynomial is the exact result?
@choice 41 | {s}^{2}
@choice 42 | {c}^{2}
@choice 43 | 1
@answer 41
@feedback 42 | The original numerator is 1-c squared, which is s squared rather than c squared.
@feedback 43 | The original numerator varies with the angle; the circle relation makes it s squared, not the constant 1.
@after sE={s}^{2},\quad E=s,\quad D:s\ne0
@wrong Check the requested operation, the original expression and its retained nonzero conditions.
@why Multiply the original fraction by nonzero s: sE=1-c squared=s squared. The proposed single factor gives s times s=s squared too. Division by s on D proves equality for every permitted angle.
@end

@question prod03_trig_identities_q02 | A complementary coordinate quotient
@template choices.v1
@version 1
@goal Simplify E to a single coordinate factor, retain its original domain and prove the identity exactly.
@given E=\frac{\left(1-{s}^{2}\right)}{c}
@domain Theta is real and measured in radians; s=sin(theta) and c=cos(theta). E names the original expression. D is its full domain, which must be retained after every cancellation.
@read prod03_trig_identities_r
@step 10 | Which condition describes the full original domain D?
@choice 12 | s\ne0
@choice 11 | c\ne0
@choice 13 | \theta\in\mathbb{R}
@answer 11
@feedback 12 | This excludes s=0, although those angles have c equal to 1 or -1 and are permitted. It also admits forbidden cosine zeros.
@feedback 13 | At theta=pi/2, c=0 and the original fraction is undefined.
@after D:c\ne0
@wrong Check the requested operation, the original expression and its retained nonzero conditions.
@why The original denominator is c, so c must be nonzero. Its zeros are pi/2 plus integer multiples of pi. No sine denominator occurs.
@step 20 | Replace the numerator by a square using the unit-circle relation.
@choice 21 | \frac{{c}^{2}}{c}
@choice 22 | \frac{{s}^{2}}{c}
@choice 23 | \frac{\left(1+{s}^{2}\right)}{c}
@answer 21
@feedback 22 | s squared is the complementary square, not 1-s squared.
@feedback 23 | Changing subtraction to addition contradicts the circle relation: c squared=1-s squared.
@after E=\frac{{c}^{2}}{c},\quad D:c\ne0
@wrong Check the requested operation, the original expression and its retained nonzero conditions.
@why Subtract s squared from s squared+c squared=1 to get 1-s squared=c squared. Keep the original denominator c.
@step 30 | Cancel one nonzero factor and leave a single coordinate factor.
@choice 32 | s
@choice 33 | 1
@choice 31 | c
@answer 31
@feedback 32 | The repeated factor is cosine; cancellation cannot change it into sine.
@feedback 33 | Dividing c squared by c removes one factor, not both.
@after E=c,\quad D:c\ne0
@wrong Check the requested operation, the original expression and its retained nonzero conditions.
@why Factor c squared as c times c. Since c is nonzero on D, cancellation leaves one c. Original cosine zeros remain excluded.
@step 40 | Multiply the original E by the nonzero multiplier c. Which polynomial is the exact result?
@choice 42 | {s}^{2}
@choice 41 | {c}^{2}
@choice 43 | 1
@answer 41
@feedback 42 | 1-s squared equals c squared, not s squared.
@feedback 43 | The original numerator is not the constant 1; its subtracted sine-square term cannot be discarded.
@after cE={c}^{2},\quad E=c,\quad D:c\ne0
@wrong Check the requested operation, the original expression and its retained nonzero conditions.
@why From the original expression, cE=1-s squared=c squared. Multiplying the proposed result c by c gives that same polynomial. Nonzero c makes this an exact reversible check.
@end

@question prod03_trig_identities_q03 | A sum of two quotients
@template choices.v1
@version 1
@goal Simplify E to one fraction with a constant numerator and a product of coordinates below it; retain D and prove the result.
@given E=\left(\tan\theta+\cot\theta\right)
@domain Theta is real and measured in radians; s=sin(theta) and c=cos(theta). E names the original expression. D is its full domain, which must be retained after every cancellation.
@read prod03_trig_identities_r
@step 10 | Which condition describes the full original domain D?
@choice 12 | s\ne0
@choice 13 | c\ne0
@choice 11 | s\ne0,\ c\ne0
@answer 11
@feedback 12 | This condition protects cotangent but still permits c=0, where tangent is undefined.
@feedback 13 | This condition protects tangent but still permits s=0, where cotangent is undefined.
@after D:s\ne0,\ c\ne0
@wrong Check the requested operation, the original expression and its retained nonzero conditions.
@why Tangent is s/c and cotangent is c/s. Both original functions must exist, so neither coordinate may be zero.
@step 20 | Rewrite both quotient functions as a sum of coordinate fractions.
@choice 22 | \left(\frac{c}{s}+\frac{c}{s}\right)
@choice 21 | \left(\frac{s}{c}+\frac{c}{s}\right)
@choice 23 | \left(\frac{s}{c}-\frac{c}{s}\right)
@answer 21
@feedback 22 | The first term is tangent, s/c; replacing it by c/s gives cotangent twice.
@feedback 23 | The original operation is addition. A difference of the same fractions changes the expression.
@after E=\left(\frac{s}{c}+\frac{c}{s}\right),\quad D:s\ne0,\ c\ne0
@wrong Check the requested operation, the original expression and its retained nonzero conditions.
@why By definition, tan(theta)=s/c and cot(theta)=c/s. Keep the plus sign between the original terms.
@step 30 | Combine the two terms over one common denominator.
@choice 31 | \frac{\left({s}^{2}+{c}^{2}\right)}{s\,c}
@choice 32 | \frac{\left(s+c\right)}{s\,c}
@choice 33 | \frac{\left({s}^{2}-{c}^{2}\right)}{s\,c}
@answer 31
@feedback 32 | Each numerator must be multiplied by the missing denominator factor, producing squares, not s+c.
@feedback 33 | The second numerator is added, not subtracted; the common-denominator operation does not change its sign.
@after E=\frac{\left({s}^{2}+{c}^{2}\right)}{s\,c},\quad D:s\ne0,\ c\ne0
@wrong Check the requested operation, the original expression and its retained nonzero conditions.
@why Multiply s/c by s/s and c/s by c/c, both equal to 1 on D. The common denominator is sc and the numerator is s squared+c squared.
@step 40 | Use the unit-circle relation to make the numerator constant.
@choice 42 | \frac{2}{s\,c}
@choice 43 | \frac{0}{s\,c}
@choice 41 | \frac{1}{s\,c}
@answer 41
@feedback 42 | The two squares add to 1, not 2.
@feedback 43 | The sum of the coordinate squares is 1, not zero; they do not cancel.
@after E=\frac{1}{s\,c},\quad D:s\ne0,\ c\ne0
@wrong Check the requested operation, the original expression and its retained nonzero conditions.
@why The numerator s squared+c squared equals exactly 1. Neither square individually equals 1; retain the nonzero denominator sc.
@step 50 | Multiply the original E by the nonzero multiplier (s times c). Which polynomial is the exact result?
@choice 52 | \left({s}^{2}+\left(1-{c}^{2}\right)\right)
@choice 51 | 1
@choice 53 | 0
@answer 51
@feedback 52 | This expression equals 2s squared, because 1-c squared=s squared. The original product contains s squared+c squared instead.
@feedback 53 | The original product is the sum of two squares, which equals 1, not zero.
@after s\,cE=1,\quad E=\frac{1}{s\,c},\quad D:s\ne0,\ c\ne0
@wrong Check the requested operation, the original expression and its retained nonzero conditions.
@why Multiply the original sum by sc: sc(s/c+c/s)=s squared+c squared=1. Both cancellations are legal on D. The proposed fraction also gives 1, proving the identity there.
@end

@question prod03_trig_identities_q04 | A difference of function squares
@template choices.v1
@version 1
@goal Reduce E to a constant on its full original domain and prove it using an exact polynomial check.
@given E=\left(\sec^{2}\theta-\tan^{2}\theta\right)
@domain Theta is real and measured in radians; s=sin(theta) and c=cos(theta). E names the original expression. D is its full domain, which must be retained after every cancellation.
@read prod03_trig_identities_r
@step 10 | Which condition describes the full original domain D?
@choice 11 | c\ne0
@choice 12 | s\ne0
@choice 13 | \theta\in\mathbb{R}
@answer 11
@feedback 12 | Both functions remain defined at s=0. Their shared forbidden zeros are those of c, not s.
@feedback 13 | At a cosine zero, both secant and tangent are undefined.
@after D:c\ne0
@wrong Check the requested operation, the original expression and its retained nonzero conditions.
@why Secant and tangent both have cosine in their denominator. Thus c nonzero is necessary and sufficient; sine zeros are allowed.
@step 20 | Rewrite the squared functions as a difference of coordinate fractions.
@choice 21 | \left(\frac{1}{{c}^{2}}-\frac{{s}^{2}}{{c}^{2}}\right)
@choice 22 | \left(\frac{1}{{s}^{2}}-\frac{{s}^{2}}{{c}^{2}}\right)
@choice 23 | \left(\frac{1}{{c}^{2}}+\frac{{s}^{2}}{{c}^{2}}\right)
@answer 21
@feedback 22 | Secant is 1/c, so its square has denominator c squared, not s squared. The tangent term alone was rewritten correctly.
@feedback 23 | The original difference is not a sum. The sign between the squared functions must remain minus.
@after E=\left(\frac{1}{{c}^{2}}-\frac{{s}^{2}}{{c}^{2}}\right),\quad D:c\ne0
@wrong Check the requested operation, the original expression and its retained nonzero conditions.
@why Square sec(theta)=1/c and tan(theta)=s/c. This gives 1/c squared minus s squared/c squared. Squaring does not change the original minus sign between terms.
@step 30 | Combine the shared denominator without changing the numerator sign.
@choice 32 | \frac{\left(1+{s}^{2}\right)}{{c}^{2}}
@choice 33 | \frac{\left({s}^{2}-1\right)}{{c}^{2}}
@choice 31 | \frac{\left(1-{s}^{2}\right)}{{c}^{2}}
@answer 31
@feedback 32 | Combining equal denominators does not turn subtraction into addition.
@feedback 33 | Reversing the numerator order negates the original expression.
@after E=\frac{\left(1-{s}^{2}\right)}{{c}^{2}},\quad D:c\ne0
@wrong Check the requested operation, the original expression and its retained nonzero conditions.
@why With equal denominators, subtract the numerators in their original order: 1-s squared, over c squared.
@step 40 | Rewrite the numerator as a square while keeping the denominator.
@choice 42 | \frac{{s}^{2}}{{c}^{2}}
@choice 41 | \frac{{c}^{2}}{{c}^{2}}
@choice 43 | \frac{{\left(1-s\right)}^{2}}{{c}^{2}}
@answer 41
@feedback 42 | s squared is not the complement of itself; the numerator is c squared.
@feedback 43 | 1-s squared is a difference of squares, not (1-s) squared; the latter contains an extra -2s term.
@after E=\frac{{c}^{2}}{{c}^{2}},\quad D:c\ne0
@wrong Check the requested operation, the original expression and its retained nonzero conditions.
@why The Pythagorean identity gives 1-s squared=c squared. The expression becomes c squared/c squared.
@step 50 | Cancel the entire nonzero denominator factor to obtain a constant.
@choice 51 | 1
@choice 52 | 0
@choice 53 | -1
@answer 51
@feedback 52 | A nonzero quantity divided by itself is 1; subtraction has already been handled in the numerator.
@feedback 53 | The numerator and denominator are the same square, not opposite numbers.
@after E=1,\quad D:c\ne0
@wrong Check the requested operation, the original expression and its retained nonzero conditions.
@why Since c is nonzero on D, c squared is nonzero and divides itself to give 1. The original excluded angles remain excluded.
@step 60 | Multiply the original E by the nonzero multiplier (c) squared. Which polynomial is the exact result?
@choice 62 | {s}^{2}
@choice 63 | 0
@choice 61 | {c}^{2}
@answer 61
@feedback 62 | The Pythagorean complement of s squared is c squared, not s squared.
@feedback 63 | 1-s squared does not vanish identically; it equals c squared.
@after {c}^{2}E={c}^{2},\quad E=1,\quad D:c\ne0
@wrong Check the requested operation, the original expression and its retained nonzero conditions.
@why Multiplying the original difference by c squared gives 1-s squared=c squared. The proposed constant 1 yields c squared too. Since c squared is nonzero on D, this proves the original identity.
@end

@question prod03_trig_identities_q05 | Two unlike denominators
@template choices.v1
@version 1
@goal Simplify E to a constant divided by a single coordinate; use a common denominator and factor before cancelling, retain D and prove the result.
@given E=\left(\frac{s}{\left(1+c\right)}+\frac{\left(1+c\right)}{s}\right)
@domain Theta is real and measured in radians; s=sin(theta) and c=cos(theta). E names the original expression. D is its full domain, which must be retained after every cancellation.
@read prod03_trig_identities_r
@step 10 | Which condition describes the full original domain D?
@choice 12 | c\ne0
@choice 11 | s\ne0
@choice 13 | \theta\in\mathbb{R}
@answer 11
@feedback 12 | Cosine may be zero: then s is 1 or -1 and both original denominators are nonzero. The necessary exclusion is s=0.
@feedback 13 | At s=0 the second fraction is undefined; at c=-1 the first is undefined too.
@after D:s\ne0
@wrong Check the requested operation, the original expression and its retained nonzero conditions.
@why The original denominators require s nonzero and 1+c nonzero. If 1+c=0 then c=-1, so s=0 by the circle relation. Thus s nonzero already ensures both denominators are nonzero.
@step 20 | Combine the original terms into one fraction without expanding products.
@choice 22 | \frac{\left({s}^{2}+\left(1+c\right)\right)}{s\,\left(1+c\right)}
@choice 21 | \frac{\left({s}^{2}+{\left(1+c\right)}^{2}\right)}{s\,\left(1+c\right)}
@choice 23 | \frac{\left({s}^{2}-{\left(1+c\right)}^{2}\right)}{s\,\left(1+c\right)}
@answer 21
@feedback 22 | The second numerator must be multiplied by 1+c as well; this requires its square, not one copy.
@feedback 23 | The two fractions were added, so their adjusted numerators must also be added.
@after E=\frac{\left({s}^{2}+{\left(1+c\right)}^{2}\right)}{s\,\left(1+c\right)},\quad D:s\ne0
@wrong Check the requested operation, the original expression and its retained nonzero conditions.
@why Multiply the first term by s/s and the second by (1+c)/(1+c). The numerator becomes s squared+(1+c) squared over s(1+c). Both new factors are nonzero on D.
@step 30 | Expand the numerator fully, leaving the common denominator unchanged.
@choice 31 | \frac{\left(\left(\left({s}^{2}+1\right)+2\,c\right)+{c}^{2}\right)}{s\,\left(1+c\right)}
@choice 32 | \frac{\left(\left({s}^{2}+1\right)+{c}^{2}\right)}{s\,\left(1+c\right)}
@choice 33 | \frac{\left(\left(\left({s}^{2}+1\right)+c\right)+{c}^{2}\right)}{s\,\left(1+c\right)}
@answer 31
@feedback 32 | The expansion omits the two cross-products c+c=2c.
@feedback 33 | There are two cross-products, so the linear term is 2c rather than c.
@after E=\frac{\left(\left(\left({s}^{2}+1\right)+2\,c\right)+{c}^{2}\right)}{s\,\left(1+c\right)},\quad D:s\ne0
@wrong Check the requested operation, the original expression and its retained nonzero conditions.
@why Use (1+c) squared=1+2c+c squared. Adding s squared gives s squared+1+2c+c squared; the denominator stays s(1+c).
@step 40 | Use the circle relation and factor the numerator to expose a cancellable binomial.
@choice 42 | \frac{\left(1+c\right)}{s\,\left(1+c\right)}
@choice 43 | \frac{2\,\left(1-c\right)}{s\,\left(1+c\right)}
@choice 41 | \frac{2\,\left(1+c\right)}{s\,\left(1+c\right)}
@answer 41
@feedback 42 | After replacing the square sum by 1, the numerator is 2+2c; factoring does not remove its factor 2.
@feedback 43 | The expanded numerator contains +2c, so the factor is 1+c, not 1-c.
@after E=\frac{2\,\left(1+c\right)}{s\,\left(1+c\right)},\quad D:s\ne0
@wrong Check the requested operation, the original expression and its retained nonzero conditions.
@why Group s squared+c squared=1. The numerator is 1+1+2c=2+2c=2(1+c). Factoring exposes the complete common factor 1+c.
@step 50 | Cancel the nonzero common binomial and leave a single coordinate denominator.
@choice 52 | \frac{1}{s}
@choice 51 | \frac{2}{s}
@choice 53 | \frac{2}{\left(1+c\right)}
@answer 51
@feedback 52 | The cancelled factor is 1+c, not the coefficient 2.
@feedback 53 | The remaining denominator factor is s; the shared 1+c is the one that cancels.
@after E=\frac{2}{s},\quad D:s\ne0
@wrong Check the requested operation, the original expression and its retained nonzero conditions.
@why On D, 1+c cannot vanish. Cancel that complete factor from 2(1+c)/[s(1+c)] to obtain 2/s, retaining the original sine exclusion.
@step 60 | Multiply the original E by the nonzero multiplier (s times (1+c)). Which polynomial is the exact result?
@choice 61 | \left(2+2\,c\right)
@choice 62 | \left(1+2\,c\right)
@choice 63 | \left(2-2\,c\right)
@answer 61
@feedback 62 | The constants are 1 from the circle square sum and 1 from the binomial expansion, totaling 2.
@feedback 63 | Expanding (1+c) squared produces +2c, not -2c.
@after s\,\left(1+c\right)E=\left(2+2\,c\right),\quad E=\frac{2}{s},\quad D:s\ne0
@wrong Check the requested operation, the original expression and its retained nonzero conditions.
@why Multiplying the original by s(1+c) gives s squared+(1+c) squared=2+2c. Multiplying 2/s by that same nonzero factor also gives 2(1+c)=2+2c. This proves equality on the retained domain.
@end

@question prod03_trig_identities_q06 | A difference with a common domain
@template choices.v1
@version 1
@goal Reduce E completely, retain every original exclusion and prove the resulting identity by clearing only nonzero denominators.
@given E=\left(\frac{c}{\left(1-s\right)}-\frac{\left(1+s\right)}{c}\right)
@domain Theta is real and measured in radians; s=sin(theta) and c=cos(theta). E names the original expression. D is its full domain, which must be retained after every cancellation.
@read prod03_trig_identities_r
@step 10 | Which condition describes the full original domain D?
@choice 12 | s\ne0
@choice 13 | \theta\in\mathbb{R}
@choice 11 | c\ne0
@answer 11
@feedback 12 | Sine may be zero here: then c is 1 or -1 and both denominators exist. The original cosine denominator is the restriction.
@feedback 13 | At c=0 the second term is undefined, including at s=1 where the first denominator also vanishes.
@after D:c\ne0
@wrong Check the requested operation, the original expression and its retained nonzero conditions.
@why The original denominators are 1-s and c. If 1-s=0 then s=1 and the circle relation forces c=0. Thus c nonzero is the full necessary and sufficient condition.
@step 20 | Combine the difference over a common denominator, keeping the subtraction order.
@choice 22 | \frac{\left({c}^{2}+\left(1+s\right)\,\left(1-s\right)\right)}{c\,\left(1-s\right)}
@choice 23 | \frac{\left({c}^{2}-\left(1+s\right)\right)}{c\,\left(1-s\right)}
@choice 21 | \frac{\left({c}^{2}-\left(1+s\right)\,\left(1-s\right)\right)}{c\,\left(1-s\right)}
@answer 21
@feedback 22 | The original second fraction is subtracted, not added.
@feedback 23 | Multiply the second numerator by its missing denominator factor 1-s before subtracting.
@after E=\frac{\left({c}^{2}-\left(1+s\right)\,\left(1-s\right)\right)}{c\,\left(1-s\right)},\quad D:c\ne0
@wrong Check the requested operation, the original expression and its retained nonzero conditions.
@why The common denominator is c(1-s). The first numerator becomes c squared; the subtracted numerator becomes (1+s)(1-s). Both denominator factors are nonzero on D.
@step 30 | Expand the numerator and distribute the outer minus sign.
@choice 32 | \frac{\left(\left({c}^{2}-1\right)-{s}^{2}\right)}{c\,\left(1-s\right)}
@choice 31 | \frac{\left(\left({c}^{2}-1\right)+{s}^{2}\right)}{c\,\left(1-s\right)}
@choice 33 | \frac{\left(\left({c}^{2}+1\right)+{s}^{2}\right)}{c\,\left(1-s\right)}
@answer 31
@feedback 32 | Subtracting -s squared gives +s squared, not -s squared.
@feedback 33 | The constant 1 belongs to the subtracted product, so it contributes -1.
@after E=\frac{\left(\left({c}^{2}-1\right)+{s}^{2}\right)}{c\,\left(1-s\right)},\quad D:c\ne0
@wrong Check the requested operation, the original expression and its retained nonzero conditions.
@why The product (1+s)(1-s)=1-s squared. Therefore c squared-(1-s squared)=c squared-1+s squared; the outer minus reverses both signs.
@step 40 | Use the circle relation in the numerator while displaying the common denominator.
@choice 41 | \frac{0}{c\,\left(1-s\right)}
@choice 42 | \frac{1}{c\,\left(1-s\right)}
@choice 43 | \frac{2}{c\,\left(1-s\right)}
@answer 41
@feedback 42 | The square sum equals 1, but the additional -1 must still be subtracted.
@feedback 43 | The numerator has a subtracted 1; it is 1-1=0, not 1+1=2.
@after E=\frac{0}{c\,\left(1-s\right)},\quad D:c\ne0
@wrong Check the requested operation, the original expression and its retained nonzero conditions.
@why The numerator is (c squared+s squared)-1=1-1=0. The denominator remains nonzero on D, so the original expression is zero there.
@step 50 | Multiply the original E by the nonzero multiplier (c times (1-s)). Which polynomial is the exact result?
@choice 52 | 1
@choice 53 | \left({c}^{2}-{s}^{2}\right)
@choice 51 | 0
@answer 51
@feedback 52 | After the two fractions are combined the square sum 1 is offset by -1, leaving 0.
@feedback 53 | The subtracted difference of squares produces +s squared in the numerator, not -s squared.
@after c\,\left(1-s\right)E=0,\quad E=0,\quad D:c\ne0
@wrong Check the requested operation, the original expression and its retained nonzero conditions.
@why The original product c(1-s)E=c squared-(1+s)(1-s)=c squared-1+s squared=0. The candidate constant 0 gives the same product. The multiplier is nonzero on D, so E=0 on D only.
@end

@question prod03_trig_identities_q07 | A coordinate fraction
@template choices.v1
@version 1
@goal Rewrite E with denominator 1+c and a single coordinate numerator by conjugate rationalization; retain D and prove the identity.
@given E=\frac{\left(1-c\right)}{s}
@domain Theta is real and measured in radians; s=sin(theta) and c=cos(theta). E names the original expression. D is its full domain, which must be retained after every cancellation.
@read prod03_trig_identities_r
@step 10 | Which condition describes the full original domain D?
@choice 11 | s\ne0
@choice 12 | c\ne0
@choice 13 | \theta\in\mathbb{R}
@answer 11
@feedback 12 | This excludes the other coordinate zeros. The actual original denominator is s, and its zero angles must be excluded.
@feedback 13 | The original fraction is undefined when s=0, regardless of how a later form behaves.
@after D:s\ne0
@wrong Check the requested operation, the original expression and its retained nonzero conditions.
@why The original denominator is s, so s must be nonzero. If 1+c were zero, the circle relation would force s=0. Thus the conjugate is also nonzero everywhere on the original domain.
@step 20 | Multiply numerator and denominator by the conjugate of the numerator.
@choice 22 | \frac{\left(1-c\right)\,\left(1-c\right)}{s\,\left(1+c\right)}
@choice 21 | \frac{\left(1-c\right)\,\left(1+c\right)}{s\,\left(1+c\right)}
@choice 23 | \frac{\left(1-c\right)\,\left(1+c\right)}{s}
@answer 21
@feedback 22 | The denominator uses 1+c but the numerator uses 1-c. These unequal multipliers do not form 1; use the same conjugate in both places.
@feedback 23 | Multiplying only the numerator changes E; the same nonzero factor must multiply the denominator.
@after E=\frac{\left(1-c\right)\,\left(1+c\right)}{s\,\left(1+c\right)},\quad D:s\ne0
@wrong Check the requested operation, the original expression and its retained nonzero conditions.
@why The conjugate of 1-c is 1+c. Multiplying by (1+c)/(1+c) preserves E because that ratio is 1 on D.
@step 30 | Apply difference of squares to the numerator product.
@choice 31 | \frac{\left(1-{c}^{2}\right)}{s\,\left(1+c\right)}
@choice 32 | \frac{\left(1+{c}^{2}\right)}{s\,\left(1+c\right)}
@choice 33 | \frac{{\left(1-c\right)}^{2}}{s\,\left(1+c\right)}
@answer 31
@feedback 32 | Difference of squares gives 1-c squared, not 1+c squared.
@feedback 33 | The two factors have opposite signs; their product is not the square of 1-c.
@after E=\frac{\left(1-{c}^{2}\right)}{s\,\left(1+c\right)},\quad D:s\ne0
@wrong Check the requested operation, the original expression and its retained nonzero conditions.
@why The opposite middle terms cancel: (1-c)(1+c)=1-c squared. The denominator is unchanged.
@step 40 | Replace the difference of squares by the complementary coordinate square.
@choice 42 | \frac{{c}^{2}}{s\,\left(1+c\right)}
@choice 43 | \frac{\left({s}^{2}-1\right)}{s\,\left(1+c\right)}
@choice 41 | \frac{{s}^{2}}{s\,\left(1+c\right)}
@answer 41
@feedback 42 | The complementary square is s squared, not c squared.
@feedback 43 | No extra -1 remains after using the Pythagorean identity.
@after E=\frac{{s}^{2}}{s\,\left(1+c\right)},\quad D:s\ne0
@wrong Check the requested operation, the original expression and its retained nonzero conditions.
@why From s squared+c squared=1, the numerator 1-c squared equals s squared.
@step 50 | Cancel the common coordinate factor to reach the requested denominator.
@choice 52 | \frac{c}{\left(1+c\right)}
@choice 51 | \frac{s}{\left(1+c\right)}
@choice 53 | \frac{1}{\left(1+c\right)}
@answer 51
@feedback 52 | The uncancelled numerator factor is s, not c.
@feedback 53 | Cancelling one s leaves the other copy in the numerator, not 1.
@after E=\frac{s}{\left(1+c\right)},\quad D:s\ne0
@wrong Check the requested operation, the original expression and its retained nonzero conditions.
@why The numerator is s times s. Cancel one nonzero s against the denominator s(1+c), leaving s/(1+c). Retain the original s nonzero restriction.
@step 60 | Multiply the original E by the nonzero multiplier (s times (1+c)). Which polynomial is the exact result?
@choice 61 | {s}^{2}
@choice 62 | {c}^{2}
@choice 63 | 1
@answer 61
@feedback 62 | The complementary square is s squared; the original product is 1-c squared.
@feedback 63 | The product is a difference of squares, not a constant 1.
@after s\,\left(1+c\right)E={s}^{2},\quad E=\frac{s}{\left(1+c\right)},\quad D:s\ne0
@wrong Check the requested operation, the original expression and its retained nonzero conditions.
@why Multiplying the original E by s(1+c) gives (1-c)(1+c)=1-c squared=s squared. Multiplying the rationalized result by the same nonzero multiplier gives s squared as well, proving the identity on D.
@end

@question prod03_trig_identities_q08 | The complementary fraction
@template choices.v1
@version 1
@goal Rewrite E with denominator 1+s and a single coordinate numerator by conjugate rationalization; retain D and prove the identity.
@given E=\frac{\left(1-s\right)}{c}
@domain Theta is real and measured in radians; s=sin(theta) and c=cos(theta). E names the original expression. D is its full domain, which must be retained after every cancellation.
@read prod03_trig_identities_r
@step 10 | Which condition describes the full original domain D?
@choice 12 | s\ne0
@choice 11 | c\ne0
@choice 13 | \theta\in\mathbb{R}
@answer 11
@feedback 12 | This excludes the other coordinate zeros. The actual original denominator is c, and its zero angles must be excluded.
@feedback 13 | The original fraction is undefined when c=0, regardless of how a later form behaves.
@after D:c\ne0
@wrong Check the requested operation, the original expression and its retained nonzero conditions.
@why The original denominator is c, so c must be nonzero. If 1+s were zero, the circle relation would force c=0. Thus the conjugate is also nonzero everywhere on the original domain.
@step 20 | Multiply numerator and denominator by the conjugate of the numerator.
@choice 22 | \frac{\left(1-s\right)\,\left(1-s\right)}{c\,\left(1+s\right)}
@choice 23 | \frac{\left(1-s\right)\,\left(1+s\right)}{c}
@choice 21 | \frac{\left(1-s\right)\,\left(1+s\right)}{c\,\left(1+s\right)}
@answer 21
@feedback 22 | The denominator uses 1+s but the numerator uses 1-s. These unequal multipliers do not form 1; use the same conjugate in both places.
@feedback 23 | Multiplying only the numerator changes E; the same nonzero factor must multiply the denominator.
@after E=\frac{\left(1-s\right)\,\left(1+s\right)}{c\,\left(1+s\right)},\quad D:c\ne0
@wrong Check the requested operation, the original expression and its retained nonzero conditions.
@why The conjugate of 1-s is 1+s. Multiplying by (1+s)/(1+s) preserves E because that ratio is 1 on D.
@step 30 | Apply difference of squares to the numerator product.
@choice 32 | \frac{\left(1+{s}^{2}\right)}{c\,\left(1+s\right)}
@choice 31 | \frac{\left(1-{s}^{2}\right)}{c\,\left(1+s\right)}
@choice 33 | \frac{{\left(1-s\right)}^{2}}{c\,\left(1+s\right)}
@answer 31
@feedback 32 | Difference of squares gives 1-s squared, not 1+s squared.
@feedback 33 | The two factors have opposite signs; their product is not the square of 1-s.
@after E=\frac{\left(1-{s}^{2}\right)}{c\,\left(1+s\right)},\quad D:c\ne0
@wrong Check the requested operation, the original expression and its retained nonzero conditions.
@why The opposite middle terms cancel: (1-s)(1+s)=1-s squared. The denominator is unchanged.
@step 40 | Replace the difference of squares by the complementary coordinate square.
@choice 41 | \frac{{c}^{2}}{c\,\left(1+s\right)}
@choice 42 | \frac{{s}^{2}}{c\,\left(1+s\right)}
@choice 43 | \frac{\left({c}^{2}-1\right)}{c\,\left(1+s\right)}
@answer 41
@feedback 42 | The complementary square is c squared, not s squared.
@feedback 43 | No extra -1 remains after using the Pythagorean identity.
@after E=\frac{{c}^{2}}{c\,\left(1+s\right)},\quad D:c\ne0
@wrong Check the requested operation, the original expression and its retained nonzero conditions.
@why From c squared+s squared=1, the numerator 1-s squared equals c squared.
@step 50 | Cancel the common coordinate factor to reach the requested denominator.
@choice 52 | \frac{s}{\left(1+s\right)}
@choice 53 | \frac{1}{\left(1+s\right)}
@choice 51 | \frac{c}{\left(1+s\right)}
@answer 51
@feedback 52 | The uncancelled numerator factor is c, not s.
@feedback 53 | Cancelling one c leaves the other copy in the numerator, not 1.
@after E=\frac{c}{\left(1+s\right)},\quad D:c\ne0
@wrong Check the requested operation, the original expression and its retained nonzero conditions.
@why The numerator is c times c. Cancel one nonzero c against the denominator c(1+s), leaving c/(1+s). Retain the original c nonzero restriction.
@step 60 | Multiply the original E by the nonzero multiplier (c times (1+s)). Which polynomial is the exact result?
@choice 62 | {s}^{2}
@choice 61 | {c}^{2}
@choice 63 | 1
@answer 61
@feedback 62 | The complementary square is c squared; the original product is 1-s squared.
@feedback 63 | The product is a difference of squares, not a constant 1.
@after c\,\left(1+s\right)E={c}^{2},\quad E=\frac{c}{\left(1+s\right)},\quad D:c\ne0
@wrong Check the requested operation, the original expression and its retained nonzero conditions.
@why Multiplying the original E by c(1+s) gives (1-s)(1+s)=1-s squared=c squared. Multiplying the rationalized result by the same nonzero multiplier gives c squared as well, proving the identity on D.
@end

@question prod03_trig_identities_q09 | A nested trigonometric quotient
@template choices.v1
@version 1
@goal Select and execute a coordinate-rewrite strategy, then rationalize to denominator 1+c with one coordinate above it. Retain the full original domain and prove the result.
@given E=\frac{\left(\sec\theta-1\right)}{\tan\theta}
@domain Theta is real and measured in radians; s=sin(theta) and c=cos(theta). E names the original expression. D is its full domain, which must be retained after every cancellation.
@read prod03_trig_identities_r
@step 10 | Which condition describes the full original domain D?
@choice 12 | s\ne0
@choice 13 | c\ne0
@choice 11 | s\ne0,\ c\ne0
@answer 11
@feedback 12 | This allows c=0, where either an original function or the outer quotient is undefined. Both coordinates must be nonzero.
@feedback 13 | This allows s=0, where either an original function or the outer quotient is undefined. Both coordinates must be nonzero.
@after D:s\ne0,\ c\ne0
@wrong Check the requested operation, the original expression and its retained nonzero conditions.
@why The original sec and tan require c nonzero. In addition, the outer division requires tan=s/c nonzero, so s must also be nonzero. Both coordinates are therefore excluded at zero.
@step 20 | Choose the strategy that exposes the nested division: replace every named trigonometric function by coordinates before cancelling.
@choice 21 | \frac{\left(\frac{1}{c}-1\right)}{\frac{s}{c}}
@choice 22 | \frac{\left(\frac{1}{s}-1\right)}{\frac{s}{c}}
@choice 23 | \frac{\left(\frac{1}{c}-1\right)}{\frac{c}{s}}
@answer 21
@feedback 22 | The reciprocal function has denominator c, not s.
@feedback 23 | The original quotient function is s/c, not its reciprocal.
@after E=\frac{\left(\frac{1}{c}-1\right)}{\frac{s}{c}},\quad D:s\ne0,\ c\ne0
@wrong Check the requested operation, the original expression and its retained nonzero conditions.
@why Use sec=1/c and tan=s/c. Rewriting both makes the division of fractions explicit, allowing the next step to use a reciprocal. No factor has yet been cancelled.
@step 30 | Divide the coordinate fractions and leave one fraction before rationalizing.
@choice 32 | \frac{\left(1+c\right)}{s}
@choice 33 | \frac{\left(1-c\right)}{c}
@choice 31 | \frac{\left(1-c\right)}{s}
@answer 31
@feedback 32 | Subtracting 1 produces numerator 1-c, not 1+c.
@feedback 33 | After reciprocal multiplication, c cancels and the remaining denominator is s.
@after E=\frac{\left(1-c\right)}{s},\quad D:s\ne0,\ c\ne0
@wrong Check the requested operation, the original expression and its retained nonzero conditions.
@why First write 1/c-1=(1-c)/c. Dividing this by s/c means multiplying by c/s. The nonzero c cancels, leaving (1-c)/s. Both original coordinate exclusions remain.
@step 40 | Choose the conjugate multiplication that prepares the requested denominator.
@choice 42 | \frac{\left(1-c\right)\,\left(1-c\right)}{s\,\left(1+c\right)}
@choice 41 | \frac{\left(1-c\right)\,\left(1+c\right)}{s\,\left(1+c\right)}
@choice 43 | \frac{\left(1-c\right)\,\left(1+c\right)}{s}
@answer 41
@feedback 42 | The numerator and denominator must use the same new factor, 1+c. Reusing 1-c only above the line changes the expression.
@feedback 43 | Multiplying only above the line is not multiplication by 1; the denominator also needs 1+c.
@after E=\frac{\left(1-c\right)\,\left(1+c\right)}{s\,\left(1+c\right)},\quad D:s\ne0,\ c\ne0
@wrong Check the requested operation, the original expression and its retained nonzero conditions.
@why Multiply by (1+c)/(1+c). This is legal: if c=-1, the circle relation would force s=0, already excluded. The numerator becomes conjugate factors.
@step 50 | Apply difference of squares and the circle relation to make the numerator one square.
@choice 51 | \frac{{s}^{2}}{s\,\left(1+c\right)}
@choice 52 | \frac{{c}^{2}}{s\,\left(1+c\right)}
@choice 53 | \frac{\left(1+{c}^{2}\right)}{s\,\left(1+c\right)}
@answer 51
@feedback 52 | The complement of c squared is s squared, not c squared.
@feedback 53 | The product of conjugates subtracts the square; addition does not represent it.
@after E=\frac{{s}^{2}}{s\,\left(1+c\right)},\quad D:s\ne0,\ c\ne0
@wrong Check the requested operation, the original expression and its retained nonzero conditions.
@why The conjugate product is 1-c squared. Since s squared+c squared=1, that is s squared. Keep s(1+c) below the line.
@step 60 | Cancel one coordinate factor and retain the original domain.
@choice 62 | \frac{1}{\left(1+c\right)}
@choice 63 | \frac{c}{\left(1+c\right)}
@choice 61 | \frac{s}{\left(1+c\right)}
@answer 61
@feedback 62 | One s survives cancellation; removing both copies incorrectly gives numerator 1.
@feedback 63 | The surviving factor is s, not the other coordinate.
@after E=\frac{s}{\left(1+c\right)},\quad D:s\ne0,\ c\ne0
@wrong Check the requested operation, the original expression and its retained nonzero conditions.
@why Cancel one s from s squared/[ s(1+c) ]. This gives s/(1+c). Even if the final form is defined at some original exclusions, neither s=0 nor c=0 is restored.
@step 70 | Multiply the original E by the nonzero multiplier (s times (1+c)). Which polynomial is the exact result?
@choice 72 | {c}^{2}
@choice 71 | {s}^{2}
@choice 73 | 1
@answer 71
@feedback 72 | The product is 1-c squared, hence s squared rather than c squared.
@feedback 73 | The conjugate product retains the subtracted square; it is not identically 1.
@after s\,\left(1+c\right)E={s}^{2},\quad E=\frac{s}{\left(1+c\right)},\quad D:s\ne0,\ c\ne0
@wrong Check the requested operation, the original expression and its retained nonzero conditions.
@why From the original nested quotient, cancel only nonzero coordinate denominators to get (1-c)/s. Multiplying by s(1+c) gives 1-c squared=s squared. The final rationalized expression gives the same polynomial. All cancelled factors are nonzero on D.
@end

@question prod03_trig_identities_q10 | A second nested quotient
@template choices.v1
@version 1
@goal Select and execute a coordinate-rewrite strategy, then rationalize to denominator 1+s with one coordinate above it. Retain the full original domain and prove the result.
@given E=\frac{\left(\csc\theta-1\right)}{\cot\theta}
@domain Theta is real and measured in radians; s=sin(theta) and c=cos(theta). E names the original expression. D is its full domain, which must be retained after every cancellation.
@read prod03_trig_identities_r
@step 10 | Which condition describes the full original domain D?
@choice 11 | s\ne0,\ c\ne0
@choice 12 | s\ne0
@choice 13 | c\ne0
@answer 11
@feedback 12 | This allows c=0, where either an original function or the outer quotient is undefined. Both coordinates must be nonzero.
@feedback 13 | This allows s=0, where either an original function or the outer quotient is undefined. Both coordinates must be nonzero.
@after D:s\ne0,\ c\ne0
@wrong Check the requested operation, the original expression and its retained nonzero conditions.
@why The original csc and cot require s nonzero. In addition, the outer division requires cot=c/s nonzero, so c must also be nonzero. Both coordinates are therefore excluded at zero.
@step 20 | Choose the strategy that exposes the nested division: replace every named trigonometric function by coordinates before cancelling.
@choice 22 | \frac{\left(\frac{1}{c}-1\right)}{\frac{c}{s}}
@choice 23 | \frac{\left(\frac{1}{s}-1\right)}{\frac{s}{c}}
@choice 21 | \frac{\left(\frac{1}{s}-1\right)}{\frac{c}{s}}
@answer 21
@feedback 22 | The reciprocal function has denominator s, not c.
@feedback 23 | The original quotient function is c/s, not its reciprocal.
@after E=\frac{\left(\frac{1}{s}-1\right)}{\frac{c}{s}},\quad D:s\ne0,\ c\ne0
@wrong Check the requested operation, the original expression and its retained nonzero conditions.
@why Use csc=1/s and cot=c/s. Rewriting both makes the division of fractions explicit, allowing the next step to use a reciprocal. No factor has yet been cancelled.
@step 30 | Divide the coordinate fractions and leave one fraction before rationalizing.
@choice 32 | \frac{\left(1+s\right)}{c}
@choice 31 | \frac{\left(1-s\right)}{c}
@choice 33 | \frac{\left(1-s\right)}{s}
@answer 31
@feedback 32 | Subtracting 1 produces numerator 1-s, not 1+s.
@feedback 33 | After reciprocal multiplication, s cancels and the remaining denominator is c.
@after E=\frac{\left(1-s\right)}{c},\quad D:s\ne0,\ c\ne0
@wrong Check the requested operation, the original expression and its retained nonzero conditions.
@why First write 1/s-1=(1-s)/s. Dividing this by c/s means multiplying by s/c. The nonzero s cancels, leaving (1-s)/c. Both original coordinate exclusions remain.
@step 40 | Choose the conjugate multiplication that prepares the requested denominator.
@choice 41 | \frac{\left(1-s\right)\,\left(1+s\right)}{c\,\left(1+s\right)}
@choice 42 | \frac{\left(1-s\right)\,\left(1-s\right)}{c\,\left(1+s\right)}
@choice 43 | \frac{\left(1-s\right)\,\left(1+s\right)}{c}
@answer 41
@feedback 42 | The numerator and denominator must use the same new factor, 1+s. Reusing 1-s only above the line changes the expression.
@feedback 43 | Multiplying only above the line is not multiplication by 1; the denominator also needs 1+s.
@after E=\frac{\left(1-s\right)\,\left(1+s\right)}{c\,\left(1+s\right)},\quad D:s\ne0,\ c\ne0
@wrong Check the requested operation, the original expression and its retained nonzero conditions.
@why Multiply by (1+s)/(1+s). This is legal: if s=-1, the circle relation would force c=0, already excluded. The numerator becomes conjugate factors.
@step 50 | Apply difference of squares and the circle relation to make the numerator one square.
@choice 52 | \frac{{s}^{2}}{c\,\left(1+s\right)}
@choice 53 | \frac{\left(1+{s}^{2}\right)}{c\,\left(1+s\right)}
@choice 51 | \frac{{c}^{2}}{c\,\left(1+s\right)}
@answer 51
@feedback 52 | The complement of s squared is c squared, not s squared.
@feedback 53 | The product of conjugates subtracts the square; addition does not represent it.
@after E=\frac{{c}^{2}}{c\,\left(1+s\right)},\quad D:s\ne0,\ c\ne0
@wrong Check the requested operation, the original expression and its retained nonzero conditions.
@why The conjugate product is 1-s squared. Since c squared+s squared=1, that is c squared. Keep c(1+s) below the line.
@step 60 | Cancel one coordinate factor and retain the original domain.
@choice 62 | \frac{1}{\left(1+s\right)}
@choice 61 | \frac{c}{\left(1+s\right)}
@choice 63 | \frac{s}{\left(1+s\right)}
@answer 61
@feedback 62 | One c survives cancellation; removing both copies incorrectly gives numerator 1.
@feedback 63 | The surviving factor is c, not the other coordinate.
@after E=\frac{c}{\left(1+s\right)},\quad D:s\ne0,\ c\ne0
@wrong Check the requested operation, the original expression and its retained nonzero conditions.
@why Cancel one c from c squared/[ c(1+s) ]. This gives c/(1+s). Even if the final form is defined at some original exclusions, neither s=0 nor c=0 is restored.
@step 70 | Multiply the original E by the nonzero multiplier (c times (1+s)). Which polynomial is the exact result?
@choice 71 | {c}^{2}
@choice 72 | {s}^{2}
@choice 73 | 1
@answer 71
@feedback 72 | The product is 1-s squared, hence c squared rather than s squared.
@feedback 73 | The conjugate product retains the subtracted square; it is not identically 1.
@after c\,\left(1+s\right)E={c}^{2},\quad E=\frac{c}{\left(1+s\right)},\quad D:s\ne0,\ c\ne0
@wrong Check the requested operation, the original expression and its retained nonzero conditions.
@why From the original nested quotient, cancel only nonzero coordinate denominators to get (1-s)/c. Multiplying by c(1+s) gives 1-s squared=c squared. The final rationalized expression gives the same polynomial. All cancelled factors are nonzero on D.
@end

@question prod03_trig_identities_q11 | A ratio of two square sums
@template choices.v1
@version 1
@goal Select a Pythagorean strategy and reduce E to one squared quotient function. Retain the full original domain and prove the result.
@given E=\frac{\left(1+\tan^{2}\theta\right)}{\left(1+\cot^{2}\theta\right)}
@domain Theta is real and measured in radians; s=sin(theta) and c=cos(theta). E names the original expression. D is its full domain, which must be retained after every cancellation.
@read prod03_trig_identities_r
@step 10 | Which condition describes the full original domain D?
@choice 12 | s\ne0
@choice 11 | s\ne0,\ c\ne0
@choice 13 | c\ne0
@answer 11
@feedback 12 | Sine nonzero does not protect tangent at c=0. Both original functions must exist.
@feedback 13 | Cosine nonzero does not protect cotangent at s=0. Both original functions must exist.
@after D:s\ne0,\ c\ne0
@wrong Check the requested operation, the original expression and its retained nonzero conditions.
@why Tangent requires c nonzero and cotangent requires s nonzero. The outer denominator 1+cot squared is positive wherever cotangent exists, so it adds no further zeros.
@step 20 | Choose the Pythagorean strategy: replace each entire square sum by one reciprocal-function square.
@choice 21 | \frac{\sec^{2}\theta}{\csc^{2}\theta}
@choice 22 | \frac{\csc^{2}\theta}{\sec^{2}\theta}
@choice 23 | \frac{\sec^{2}\theta}{\cot^{2}\theta}
@answer 21
@feedback 22 | This reverses numerator and denominator; the tangent sum belongs above the fraction line.
@feedback 23 | The denominator is 1+cot squared, which is csc squared, not cot squared alone.
@after E=\frac{\sec^{2}\theta}{\csc^{2}\theta},\quad D:s\ne0,\ c\ne0
@wrong Check the requested operation, the original expression and its retained nonzero conditions.
@why Dividing s squared+c squared=1 by c squared gives 1+tan squared=sec squared. Dividing by s squared gives 1+cot squared=csc squared. Apply these to numerator and denominator in their original order.
@step 30 | Replace both reciprocal squares by their coordinate definitions.
@choice 32 | \frac{\frac{1}{{s}^{2}}}{\frac{1}{{c}^{2}}}
@choice 33 | \frac{\frac{1}{{c}^{2}}}{{s}^{2}}
@choice 31 | \frac{\frac{1}{{c}^{2}}}{\frac{1}{{s}^{2}}}
@answer 31
@feedback 32 | The two reciprocal definitions have been interchanged.
@feedback 33 | Cosecant squared is the reciprocal of s squared, not s squared itself.
@after E=\frac{\frac{1}{{c}^{2}}}{\frac{1}{{s}^{2}}},\quad D:s\ne0,\ c\ne0
@wrong Check the requested operation, the original expression and its retained nonzero conditions.
@why Sec squared is 1/c squared and csc squared is 1/s squared. Their quotient is a fraction divided by another fraction.
@step 40 | Invert the divisor and simplify to a ratio of coordinate squares.
@choice 42 | \frac{{c}^{2}}{{s}^{2}}
@choice 41 | \frac{{s}^{2}}{{c}^{2}}
@choice 43 | \frac{1}{{s}^{2}\,{c}^{2}}
@answer 41
@feedback 42 | Inverting the whole expression instead of only its divisor gives the reciprocal result.
@feedback 43 | Multiplication by the reciprocal of 1/s squared places s squared above the line, not below it.
@after E=\frac{{s}^{2}}{{c}^{2}},\quad D:s\ne0,\ c\ne0
@wrong Check the requested operation, the original expression and its retained nonzero conditions.
@why Dividing 1/c squared by 1/s squared means multiplying 1/c squared by s squared. Thus the quotient is s squared/c squared. Both squares are nonzero on D.
@step 50 | Express the coordinate-square ratio as one squared quotient function.
@choice 51 | \tan^{2}\theta
@choice 52 | \cot^{2}\theta
@choice 53 | \sec^{2}\theta
@answer 51
@feedback 52 | Cotangent squared is c squared/s squared, the reciprocal ratio.
@feedback 53 | Secant squared has numerator 1, not s squared.
@after E=\tan^{2}\theta,\quad D:s\ne0,\ c\ne0
@wrong Check the requested operation, the original expression and its retained nonzero conditions.
@why Since tan(theta)=s/c, squaring gives tan squared=s squared/c squared. This changes notation but not the retained domain s nonzero and c nonzero.
@step 60 | Multiply the original E by the nonzero multiplier (c) squared. Which polynomial is the exact result?
@choice 62 | {c}^{2}
@choice 63 | 1
@choice 61 | {s}^{2}
@answer 61
@feedback 62 | Multiplying by c squared leaves the original numerator s squared after the reciprocal division.
@feedback 63 | The numerator of tangent squared is s squared, not 1; the original ratio is not secant squared.
@after {c}^{2}E={s}^{2},\quad E=\tan^{2}\theta,\quad D:s\ne0,\ c\ne0
@wrong Check the requested operation, the original expression and its retained nonzero conditions.
@why Using the original square sums, E=[(c squared+s squared)/c squared]/[(s squared+c squared)/s squared]=s squared/c squared, because each square sum is 1. Therefore c squared E=s squared. Multiplying tan squared by c squared gives the same result on D.
@end

@question prod03_trig_identities_q12 | Two reciprocal binomials
@template choices.v1
@version 1
@goal Choose and execute a common-denominator strategy, then express E as a constant multiple of one reciprocal-function square. Retain D and prove the identity.
@given E=\left(\frac{1}{\left(1-s\right)}+\frac{1}{\left(1+s\right)}\right)
@domain Theta is real and measured in radians; s=sin(theta) and c=cos(theta). E names the original expression. D is its full domain, which must be retained after every cancellation.
@read prod03_trig_identities_r
@step 10 | Which condition describes the full original domain D?
@choice 12 | s\ne0
@choice 13 | \theta\in\mathbb{R}
@choice 11 | c\ne0
@answer 11
@feedback 12 | Sine zero is allowed: both original denominators are then 1. The forbidden sine values are the extremes, where cosine is zero.
@feedback 13 | At s=1 or s=-1 one of the original denominators vanishes.
@after D:c\ne0
@wrong Check the requested operation, the original expression and its retained nonzero conditions.
@why The original denominators require s not equal to 1 and s not equal to -1. On the unit circle these are exactly the points where c=0. Thus c nonzero is necessary and sufficient.
@step 20 | Choose the common-denominator strategy that keeps both original fractions equivalent.
@choice 22 | \frac{\left(\left(1-s\right)+\left(1-s\right)\right)}{\left(1-s\right)\,\left(1+s\right)}
@choice 21 | \frac{\left(\left(1+s\right)+\left(1-s\right)\right)}{\left(1-s\right)\,\left(1+s\right)}
@choice 23 | \frac{\left(\left(1+s\right)-\left(1-s\right)\right)}{\left(1-s\right)\,\left(1+s\right)}
@answer 21
@feedback 22 | The first term needs its missing factor 1+s, not another 1-s.
@feedback 23 | The original terms are added. Subtracting the adjusted numerators changes the operation.
@after E=\frac{\left(\left(1+s\right)+\left(1-s\right)\right)}{\left(1-s\right)\,\left(1+s\right)},\quad D:c\ne0
@wrong Check the requested operation, the original expression and its retained nonzero conditions.
@why The common denominator is (1-s)(1+s). The first numerator gains 1+s; the second gains 1-s. Adding gives (1+s)+(1-s) above that product. Both factors are nonzero on D.
@step 30 | Collect the numerator terms while leaving the denominator product intact.
@choice 31 | \frac{2}{\left(1-s\right)\,\left(1+s\right)}
@choice 32 | \frac{2\,s}{\left(1-s\right)\,\left(1+s\right)}
@choice 33 | \frac{0}{\left(1-s\right)\,\left(1+s\right)}
@answer 31
@feedback 32 | The coordinate terms have opposite signs and cancel; it is the two constants that add.
@feedback 33 | Only s-s cancels. The remaining 1+1 contributes 2.
@after E=\frac{2}{\left(1-s\right)\,\left(1+s\right)},\quad D:c\ne0
@wrong Check the requested operation, the original expression and its retained nonzero conditions.
@why Expand the numerator: 1+s+1-s. The coordinate terms cancel and the constants add to 2.
@step 40 | Apply difference of squares to the denominator product.
@choice 42 | \frac{2}{\left(1+s\right)}
@choice 43 | \frac{2}{{\left(1-s\right)}^{2}}
@choice 41 | \frac{2}{\left(1-{s}^{2}\right)}
@answer 41
@feedback 42 | The factor 1-s cannot be dropped. Multiplying both conjugates gives 1-s squared, not one surviving binomial.
@feedback 43 | The factors are conjugates, not two copies of 1-s. A square would introduce an extra -2s term.
@after E=\frac{2}{\left(1-{s}^{2}\right)},\quad D:c\ne0
@wrong Check the requested operation, the original expression and its retained nonzero conditions.
@why The product (1-s)(1+s)=1-s squared. Its middle terms +s and -s cancel; the product of -s and +s is -s squared.
@step 50 | Replace the denominator difference by one coordinate square.
@choice 52 | \frac{2}{{s}^{2}}
@choice 51 | \frac{2}{{c}^{2}}
@choice 53 | \frac{2}{c}
@answer 51
@feedback 52 | The complement of s squared is c squared, not s squared.
@feedback 53 | The identity gives c squared, not c; no square root operation is being performed.
@after E=\frac{2}{{c}^{2}},\quad D:c\ne0
@wrong Check the requested operation, the original expression and its retained nonzero conditions.
@why The circle relation gives 1-s squared=c squared. That square is nonzero because c is nonzero on D.
@step 60 | Write the result as a constant multiple of one reciprocal-function square.
@choice 61 | 2\,\sec^{2}\theta
@choice 62 | 2\,\csc^{2}\theta
@choice 63 | \sec^{2}\theta
@answer 61
@feedback 62 | Cosecant squared has denominator s squared, not c squared.
@feedback 63 | The coefficient 2 remains after replacing the reciprocal square; changing notation does not remove it.
@after E=2\,\sec^{2}\theta,\quad D:c\ne0
@wrong Check the requested operation, the original expression and its retained nonzero conditions.
@why Since sec(theta)=1/c, sec squared=1/c squared. Therefore 2/c squared=2sec squared, with the coefficient 2 unchanged and the original domain retained.
@step 70 | Multiply the original E by the nonzero multiplier (c) squared. Which polynomial is the exact result?
@choice 72 | 0
@choice 73 | 2\,s
@choice 71 | 2
@answer 71
@feedback 72 | The coordinate terms cancel, but the two constants give 2.
@feedback 73 | The numerator has +s and -s, not two positive sine terms.
@after {c}^{2}E=2,\quad E=2\,\sec^{2}\theta,\quad D:c\ne0
@wrong Check the requested operation, the original expression and its retained nonzero conditions.
@why In the original expression, c squared=(1-s)(1+s). Multiplying each original term by c squared gives (1+s)+(1-s)=2. The final 2sec squared also gives 2 after multiplication by c squared. This nonzero multiplier proves equality everywhere on D.
@end
