#!/usr/bin/env python3
"""Eight authored follow-up questions; textbook references contain no answer keys."""
import argparse
import hashlib
import json
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
OUTPUT = ROOT / 'content/corpus/matrix_reasoning.json'


def matrix(rows, augmented=False):
    body = r'\\'.join('&'.join(map(str, row)) for row in rows)
    if augmented:
        return r'\left[\begin{array}{rr|r}' + body + r'\end{array}\right]'
    return r'\begin{bmatrix}' + body + r'\end{bmatrix}'


def decision(prompt, correct, wrong, after, explanation, purpose='operation_choice'):
    return dict(prompt=prompt, correct=correct, wrong=wrong, after=after, explanation=explanation, purpose=purpose)


def build():
    authored = []

    def question(title, task, given, decisions, refs, check):
        key = f'matrix_reasoning_{len(authored)+1:02}'
        states = [dict(id=1, display=given)]
        steps = []
        for i, step in enumerate(decisions):
            labels = [step['correct'], *step['wrong']]
            assert len(set(labels)) == len(labels)
            rotation = int(hashlib.sha256(f'{key}/{i}'.encode()).hexdigest()[:8], 16) % len(labels)
            labels = labels[rotation:] + labels[:rotation]
            states.append(dict(id=i+2, display=step['after']))
            steps.append(dict(id=i+1, layer_name=f'Step {i+1}', prompt=step['prompt'],
                options=[dict(id=j+1, label=label) for j, label in enumerate(labels)],
                accepted_option_ids=[labels.index(step['correct'])+1],
                wrong_hint='That tile does not meet this step. The working is unchanged.',
                explanation=step['explanation'], hint=step['prompt'], next_move=step['correct'],
                semantics=dict(purpose=step['purpose'], completion='any_accepted', before=i+1, after=i+2)))
        authored.append(dict(id=key, title=f'Matrix reasoning {len(authored)+1:02} / {title}',
            level='practice', subject='linear_algebra', topic='topic_0094', reading_refs=refs, check=check,
            question=dict(schema_version=1, id=key, content_version=1, equation=given, skill=title,
                description=task, working_states=states, steps=steps)))

    a = matrix([[1, 2], [3, 7]])
    upper = matrix([[1, 2], [0, 1]])
    identity = matrix([[1, 0], [0, 1]])
    question('Clear below, then above', 'Find the RREF of A. All displayed columns belong to A.', a, [
        decision('Clear the 3 below the first pivot.', r'R_2\leftarrow R_2-3R_1',
                 [r'R_2\leftarrow R_2-R_1', r'R_2\leftarrow R_2+3R_1'], upper,
                 'Subtract three times every entry of row 1: [3,7]-3[1,2]=[0,1].'),
        decision('Clear the 2 above the second pivot.', r'R_1\leftarrow R_1-2R_2',
                 [r'R_1\leftarrow R_1+2R_2', r'R_2\leftarrow R_2-2R_1'], identity,
                 'Row 1 becomes [1,0]. Both pivots are 1 and alone in their columns, so this is RREF.')],
        ['rref.operations', 'rref.reduced'], dict(kind='rref', given=[[1,2],[3,7]], answer=[[1,0],[0,1]]))

    a = matrix([[0, -2], [1, 3]])
    question('Swap and scale', 'Reduce A to RREF. Move a usable pivot into the first row.', a, [
        decision('Put the leading 1 in the first row.', r'R_1\leftrightarrow R_2',
                 [r'R_1\leftarrow 0R_1', r'R_2\leftarrow R_2+R_1'], matrix([[1,3],[0,-2]]),
                 'Swap the complete rows. Multiplying a row by zero is not reversible.'),
        decision('Make the second pivot equal to 1.', r'R_2\leftarrow-\frac12 R_2',
                 [r'R_2\leftarrow\frac12 R_2', r'R_2\leftarrow-2R_2'], matrix([[1,3],[0,1]]),
                 'Multiplying [0,-2] by -1/2 gives [0,1].'),
        decision('Clear the entry above that pivot.', r'R_1\leftarrow R_1-3R_2',
                 [r'R_1\leftarrow R_1+3R_2', r'R_2\leftarrow R_2-3R_1'], identity,
                 'The second column becomes [0,1], while the first remains [1,0].')],
        ['rref.operations', 'rref.equivalence', 'rref.reduced'], dict(kind='rref', given=[[0,-2],[1,3]], answer=[[1,0],[0,1]]))

    a = matrix([[2,1,3],[0,3,6]])
    question('Fractional pivots', 'Find the RREF of this coefficient matrix A. Its third column is not a right-hand side.', a, [
        decision('Normalize the first pivot.', r'R_1\leftarrow\frac12 R_1',
                 [r'R_1\leftarrow 2R_1', r'R_1\leftarrow-\frac12 R_1'], matrix([[1,r'\frac12',r'\frac32'],[0,3,6]]),
                 'Scaling the full first row gives [1,1/2,3/2].'),
        decision('Normalize the second pivot.', r'R_2\leftarrow\frac13 R_2',
                 [r'R_2\leftarrow 3R_2', r'R_2\leftarrow\frac12 R_2'], matrix([[1,r'\frac12',r'\frac32'],[0,1,2]]),
                 'The second row becomes [0,1,2].'),
        decision('Clear 1/2 above the second pivot.', r'R_1\leftarrow R_1-\frac12 R_2',
                 [r'R_1\leftarrow R_1-2R_2', r'R_1\leftarrow R_1+\frac12 R_2'], matrix([[1,0,r'\frac12'],[0,1,2]]),
                 'The last entry becomes 3/2-(1/2)2=1/2. Non-pivot columns may retain nonzero entries.')],
        ['matrix.entries', 'rref.operations', 'rref.reduced'], dict(kind='rref', given=[[2,1,3],[0,3,6]], answer=[[1,0,'1/2'],[0,1,2]]))

    b = matrix([[1,3,0],[0,1,2]])
    question('REF is not always RREF', 'Classify B, identify the obstruction, then finish reducing it.', b, [
        decision('Which conditions hold?', r'\mathrm{REF},\ \neg\mathrm{RREF}',
                 [r'\mathrm{RREF}', r'\neg\mathrm{REF}'], b,
                 'The pivots move right and entries below them are zero. The 3 above the second pivot prevents RREF.', 'verification'),
        decision('Which entry violates the reduced condition?', r'b_{12}=3',
                 [r'b_{23}=2', r'b_{13}=0'], b,
                 'Column 2 is a pivot column, so its 3 must be cleared. Column 3 has no pivot and may contain 2.', 'verification'),
        decision('Clear that entry.', r'R_1\leftarrow R_1-3R_2',
                 [r'R_1\leftarrow R_1+3R_2', r'R_2\leftarrow R_2-3R_1'], matrix([[1,0,-6],[0,1,2]]),
                 'Row 1 becomes [1,0,-6]. Both pivot columns now meet the reduced conditions.')],
        ['rref.echelon', 'rref.reduced'], dict(kind='rref', given=[[1,3,0],[0,1,2]], answer=[[1,0,-6],[0,1,2]]))

    a = matrix([[1,2,-1],[2,4,-2]])
    reduced = matrix([[1,2,-1],[0,0,0]])
    question('Two free variables', 'Solve Ax=0. A is a coefficient matrix: all three columns represent unknowns.', a, [
        decision('Eliminate the dependent second row.', r'R_2\leftarrow R_2-2R_1',
                 [r'R_2\leftarrow R_2+2R_1', r'R_2\leftarrow R_2-R_1'], reduced,
                 'The second equation was twice the first. Its zero row imposes no extra condition.'),
        decision('Choose the pivot-column set.', r'\{1\}', [r'\{1,2\}', r'\{1,3\}'], reduced,
                 'Only column 1 has a pivot. Columns 2 and 3 are free-variable columns.', 'verification'),
        decision('Set x2=s and x3=t. Choose x.', matrix([['-2s+t'],['s'],['t']]),
                 [matrix([['2s-t'],['s'],['t']]), matrix([['s'],['t'],['0']])], r'x='+matrix([['-2s+t'],['s'],['t']]),
                 'The remaining equation is x1+2x2-x3=0. Thus x1=-2s+t, with s and t arbitrary real numbers.', 'calculation')],
        ['matrix.entries', 'rref.echelon', 'rref.reduced'], dict(kind='kernel', given=[[1,2,-1],[2,4,-2]], basis=[[-2,1,0],[1,0,1]]))

    a = matrix([[1,1,5],[2,3,12]], True)
    question('Carry the right-hand side', 'Solve for x and y. The column after the bar is the right-hand side b.', a, [
        decision('Eliminate the 2 below the first pivot.', r'R_2\leftarrow R_2-2R_1',
                 [r'R_2\leftarrow R_2-R_1', r'R_2\leftarrow R_2+2R_1'], matrix([[1,1,5],[0,1,2]], True),
                 'The right-hand side changes too: 12-2(5)=2. The second equation is y=2.'),
        decision('Clear the y term in the first row.', r'R_1\leftarrow R_1-R_2',
                 [r'R_1\leftarrow R_1+R_2', r'R_2\leftarrow R_2-R_1'], matrix([[1,0,3],[0,1,2]], True),
                 'Subtracting the full second row gives x=5-2=3.'),
        decision('Read the ordered solution (x,y).', r'(3,2)', [r'(2,3)', r'(5,2)'], r'(x,y)=(3,2)',
                 'Substitution verifies 3+2=5 and 2(3)+3(2)=12.', 'verification')],
        ['matrix.entries', 'rref.operations', 'rref.preservation'], dict(kind='unique', given=[[1,1],[2,3]], rhs=[5,12], answer=[3,2]))

    a = matrix([[1,2,3],[2,4,8]], True)
    reduced = matrix([[1,2,3],[0,0,2]], True)
    question('Detect no solution', 'Determine the solution set of the augmented system [A | b].', a, [
        decision('Compare the second equation with twice the first.', r'R_2\leftarrow R_2-2R_1',
                 [r'R_2\leftarrow R_2+2R_1', r'R_2\leftarrow R_2-R_1'], reduced,
                 'The coefficients cancel, but the right-hand side becomes 8-2(3)=2.'),
        decision('Read the second row as an equation.', r'0=2', [r'y=2', r'0=0'], reduced,
                 'Both unknown coefficients are zero. The last entry is explicitly a right-hand side.', 'verification'),
        decision('Choose the solution set.', r'\varnothing', [r'\mathbb R^2', r'\{(0,0)\}'], r'S=\varnothing',
                 'No values of the unknowns make 0=2 true, so the system is inconsistent.', 'verification')],
        ['matrix.entries', 'rref.operations', 'rref.preservation'], dict(kind='inconsistent', given=[[1,2],[2,4]], rhs=[3,8]))

    a = matrix([[1,-1,2],[3,-3,6]], True)
    reduced = matrix([[1,-1,2],[0,0,0]], True)
    question('One free variable', 'Describe every solution of the augmented system. Use a real parameter t.', a, [
        decision('Remove the dependent second equation.', r'R_2\leftarrow R_2-3R_1',
                 [r'R_2\leftarrow R_2-R_1', r'R_2\leftarrow R_2+3R_1'], reduced,
                 'All three entries cancel, including 6-3(2)=0.'),
        decision('What does the zero row say?', r'0=0', [r'0=2', r'y=0'], reduced,
                 'The second row adds no restriction. It does not require y=0.', 'verification'),
        decision('Set y=t. Choose the ordered pair (x,y).', r'(t+2,t)', [r'(t-2,t)', r'(2,t)'], r'(x,y)=(t+2,t),\quad t\in\mathbb R',
                 'The remaining equation is x-y=2, so x=t+2. Here there are two unknowns; the third column is b.', 'calculation')],
        ['matrix.entries', 'rref.operations', 'rref.preservation'], dict(kind='affine', given=[[1,-1],[3,-3]], rhs=[2,6], origin=[2,0], direction=[1,1]))
    return dict(schema_version=1, collection='matrix_reasoning', questions=authored)


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--check', action='store_true')
    args = parser.parse_args()
    bank = build(); text = json.dumps(bank, ensure_ascii=False, indent=2) + '\n'
    if args.check:
        assert OUTPUT.read_text() == text, 'Matrix reasoning differs from its authored source'
    else:
        OUTPUT.write_text(text)
    print(f"{len(bank['questions'])} matrix reasoning questions; {sum(len(q['question']['steps']) for q in bank['questions'])} decisions")
