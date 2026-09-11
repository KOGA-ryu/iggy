"""Whole-pool exact oracle, actual compiler mutations, and final receipt checks."""
import copy
from fractions import Fraction as F
import json
from pathlib import Path
import re
import subprocess
import tempfile
from unittest.mock import patch

import build_question_batch as batch
import export_learning as export
import generate as g


def oracle_coordinate(family, theta):
    """Triangle/quadrant folding, independent of producer branches and rotations."""
    t=(theta+(F(1,2) if family=='cosine_turn' else 0))%2
    sign=1
    if t>1: t-=1; sign=-1
    if t>F(1,2): t=1-t
    rational,radical={F(0):(F(0),F(0)),F(1,6):(F(1,2),F(0)),
                     F(1,3):(F(0),F(1,2)),F(1,2):(F(1),F(0))}[t]
    return sign*rational,sign*radical


def oracle(q):
    c=q['case']; a,b,rhs=(F(c[n]) for n in ('a','b','c')); level=(rhs-b)/a
    # All supported coordinates arise from axes or 30/60-degree triangles.
    # Membership in this lattice plus the circle intersection count proves
    # completeness in the continuum, not merely in the enumerated lattice.
    answers=[F(n,6) for n in range(12) if oracle_coordinate(c['family'],F(n,6))==(level,0)]
    assert len(answers)==(1 if abs(level)==1 else 2)
    assert all(a*oracle_coordinate(c['family'],t)[0]+b==rhs for t in answers)
    return level,answers


def rejected(fn):
    try: fn()
    except (export.ExportError,ValueError,KeyError,TypeError,ZeroDivisionError) as e:
        return str(e)
    raise AssertionError('Deliberately corrupted content passed its check')


def replay(documents,model):
    with tempfile.TemporaryDirectory(prefix='trig-test-',dir='/private/tmp') as name:
        folder=Path(name)
        export.write_tree(folder,documents)
        result=subprocess.run([str(model),'--question-batch',str(folder)],capture_output=True,text=True,timeout=60)
        assert result.returncode==0,result.stderr or result.stdout
        return export.decoded(result.stdout)


def compile_template(family,set_name,template,model,values=None):
    # This is a real editable Markdown file, not a mutation of expected output.
    # Use precisely the shared manifest/template/chapter assembly boundary.
    with tempfile.TemporaryDirectory(prefix='trig-template-',dir='/private/tmp') as name:
        source=Path(name)
        export.write_tree(source,{'sequence.json':export.encoded(g.sequence(family,set_name)),
                                 'lesson.md.in':g.lesson(family).encode(),
                                 'questions.paths.md.in':template.encode()})
        qs,certs,docs,_,_=batch.reasoning_documents(source,g.CHECKERS,
                         values or g.presentation(family,set_name),'chapter.paths.md')
        return qs,certs,replay(docs,model),docs


def check_whole_pool():
    records=[]; boundary_rejections=[]
    actual_read=export.read_bytes
    for field,value in [('units','degrees'),('upper','3'),('include_upper',True),('include_lower',False)]:
        corrupt=g.recipe(); corrupt['domain'][field]=value
        with patch.object(export,'read_bytes',side_effect=lambda p,*a:export.encoded(corrupt) if p==g.SOURCE/'recipe.json' else actual_read(p,*a)):
            boundary_rejections.append(rejected(g.recipe))
    for family in g.FAMILIES:
        # All twelve angle lattice points plus negative/positive full-turn copies.
        for n in range(-24,37):
            assert g.coordinate(family,F(n,6))==oracle_coordinate(family,F(n,6))
        for set_name in g.SETS:
            for q in g.sequence(family,set_name)['questions']:
                level,angles=oracle(q); cert=batch.reasoning_certificate(q,g.CHECKERS)
                facts=cert['evidence']['facts']
                assert list(map(F,facts['interval_solutions_theta_over_pi']))==angles
                assert F(facts['level'])==level
                c=q['case']; a,b,rhs=[F(c[k]) for k in ('a','b','c')]; alpha=F(c['alpha_pi'])
                # Independently determine each semantic option's goal-validity.
                role=q['role']
                if role=='read_notation':
                    assert [a*v+b==rhs for v in (level,-level,level+F(1,2))]==[True,False,False]
                elif role=='worked_check':
                    other=next(t for t in angles if t!=alpha)
                    assert [t in angles and t!=alpha for t in (alpha,other,(alpha+F(1,2))%2)]==[False,True,False]
                elif role=='choose_next_step':
                    triples=[(F(1),level),(a,rhs-b),(F(1),level+F(1,2))]
                    equivalence=[coef*level==val for coef,val in triples]
                    goal=[eq and coef==1 for eq,(coef,val) in zip(equivalence,triples)]
                    assert equivalence==[True,True,False] and goal==[True,False,False]
                elif role=='explain_step':
                    transformations=[alpha+1,(2 if family=='sine_turn' else 1)-alpha,
                                     (1 if family=='sine_turn' else 2)-alpha]
                    assert [oracle_coordinate(family,t)==(level,0) for t in transformations]==[False,False,True]
                    assert len({t%2 for t in transformations})==3
                elif role=='repair_error':
                    assert cert['expected']['after'][0]=='L_2'
                    assert g.angle_set(angles) not in cert['expected']['given'].split('L_2:')[0]
                    proposed=[angles[:-1],sorted(angles+[F(2)]),angles]
                    assert [s==angles for s in proposed]==[False,False,True]
                elif role=='independent':
                    contrast=-level if level else F(1,2)
                    assert a*contrast+b!=rhs and angles[:-1]!=angles
                records.append({'id':q['id'],'role':role,'level':str(level),
                                'angles':[str(t) for t in angles],'decisions':cert['steps_checked']})
                bad=copy.deepcopy(q); bad['case']['a']=0
                boundary_rejections.append(rejected(lambda:batch.reasoning_certificate(bad,g.CHECKERS)))
                bad=copy.deepcopy(q); bad['case']['c']=9
                boundary_rejections.append(rejected(lambda:batch.reasoning_certificate(bad,g.CHECKERS)))
                for field,value in [('a',True),('alpha_pi',True),('alpha_pi','5/7')]:
                    bad=copy.deepcopy(q); bad['case'][field]=value
                    boundary_rejections.append(rejected(lambda:batch.reasoning_certificate(bad,g.CHECKERS)))
                bad=copy.deepcopy(q); bad['case'].update(a=2,b=0,c=3)
                boundary_rejections.append(rejected(lambda:batch.reasoning_certificate(bad,g.CHECKERS)))
                # Missing, duplicate, coterminal, excluded endpoint, wrong quadrant.
                mutations=[angles[:-1],angles+[angles[0]],angles+[angles[0]+2],
                           angles+[F(2)],[(t+F(1,2))%2 for t in angles]]
                for corrupt in mutations:
                    boundary_rejections.append(rejected(lambda:g.verify_solutions(family,level,corrupt)))
        for level in map(F,('0','1/2','-1/2','1','-1')):
            answers=g.solutions(family,level)
            if F(0) in answers:
                boundary_rejections.append(rejected(lambda:g.verify_solutions(family,level,[t for t in answers if t])))
            # Corrupt the producer's branch table; exact rotations/count reject it.
            original=g.BRANCHES[family][level]
            try:
                g.BRANCHES[family][level]=tuple((t+F(1,6))%2 for t in original)
                boundary_rejections.append(rejected(lambda:g.solutions(family,level)))
            finally: g.BRANCHES[family][level]=original
    assert len(records)==36 and sum(r['decisions'] for r in records)==42
    return records,boundary_rejections


def compiler_mutations(model):
    evidence=[]
    for family in g.FAMILIES:
        path=g.SOURCE/'families'/family/'questions.paths.md.in'
        original=export.read_bytes(path).decode()
        qs,certs,baseline,docs=compile_template(family,'teaching',original,model)
        batch.verify_role_content(qs,baseline['questions'],certs)
        for i,role in enumerate(g.ROLES):
            # Locate the actual source role section; no generated-document parser.
            start=original.index('@question {{'+role+'_id}}')
            end=original.index('\n@end',start)
            section=original[start:end]
            prompt=re.search(r'^@step 10 \| (.+)$',section,re.M).group(1)
            marker='Recheck this exact task: '+prompt
            altered=original[:start]+section.replace('@step 10 | '+prompt,'@step 10 | '+marker,1)+original[end:]
            eq,ec,changed,changed_docs=compile_template(family,'teaching',altered,model)
            batch.verify_role_content(eq,changed['questions'],ec)
            assert changed['questions'][i]['question']['steps'][0]['prompt']==marker
            before=copy.deepcopy(baseline['questions']); after=copy.deepcopy(changed['questions'])
            after[i]['question']['steps'][0]['prompt']=prompt
            assert before==after, 'Prompt edit unexpectedly changed mathematics or another field'
            evidence.append(dict(family=family,role=role,mutation='real_markdown_prompt',accepted=True,
                                 before_sha256=export.sha(docs['chapter.paths.md']),
                                 after_sha256=export.sha(changed_docs['chapter.paths.md']),
                                 compiled_prompt=marker))
            # Individual feedback is also independently editable Markdown prose.
            match=re.search(r'^@feedback (\d+) \| (.+)$',section,re.M)
            feedback_marker='Check this choice specifically. '+match.group(2)
            altered=original[:start]+section.replace(match.group(0),f'@feedback {match.group(1)} | '+feedback_marker,1)+original[end:]
            eq,ec,changed,_=compile_template(family,'teaching',altered,model)
            batch.verify_role_content(eq,changed['questions'],ec)
            choice=next(o for o in changed['questions'][i]['question']['steps'][0]['options'] if o['id']==int(match.group(1)))
            assert choice['wrong_feedback']==batch.fill_template(feedback_marker,g.presentation(family,'teaching'),path)
            evidence.append(dict(family=family,role=role,mutation='real_markdown_feedback',accepted=True))
            for number,step in enumerate(certs[i]['expected']['steps'],1):
                # Compile deliberately false keys with structurally valid feedback.
                values=g.presentation(family,'teaching')
                field=role+'_choices_'+str(number); text=values[field]
                old_id=10*number+step['choices'].index(step['answer'])+1
                new_id=next(10*number+j+1 for j,label in enumerate(step['choices']) if label!=step['answer'])
                values[field]=text.replace('@answer '+str(old_id),'@answer '+str(new_id))
                malformed=re.sub(r'^@feedback '+str(new_id)+r' \|.*\n','',section,flags=re.M)
                malformed=malformed.replace('{{'+field+'}}','{{'+field+'}}\n@feedback '+str(old_id)+' | Deliberate false-key fixture.')
                altered=original[:start]+malformed+original[end:]
                eq,ec,changed,_=compile_template(family,'teaching',altered,model,values)
                error=rejected(lambda:batch.verify_role_content(eq,changed['questions'],ec))
                assert 'key disagree' in error
                evidence.append(dict(family=family,role=role,step=number,mutation='compiled_false_key',rejected=error))
                values=g.presentation(family,'teaching')
                values[role+'_after_'+str(number)]=r'\theta=99\pi'
                eq,ec,changed,_=compile_template(family,'teaching',original,model,values)
                error=rejected(lambda:batch.verify_role_content(eq,changed['questions'],ec))
                assert 'working disagree' in error
                evidence.append(dict(family=family,role=role,step=number,mutation='compiled_false_working',rejected=error))
        # Equivalent mathematical labels must fail against the canonical certificate.
        corrupt=copy.deepcopy(baseline['questions'])
        options=corrupt[0]['question']['steps'][0]['options']
        accepted=corrupt[0]['question']['steps'][0]['accepted_option_ids'][0]
        wrong=next(o for o in options if o['id']!=accepted)
        correct=next(o for o in options if o['id']==accepted)
        wrong['label']=correct['label'].replace(r'\frac{1}{2}',r'\frac{2}{4}')
        evidence.append(dict(family=family,mutation='equivalent_compiled_option',
                             rejected=rejected(lambda:batch.verify_role_content(qs,corrupt,certs))))
    return evidence


def final_receipts(production,model):
    patterns=[]; rows=[]
    assert production['source_sha256']==g.source_hashes(), 'Regenerate stale production receipt'
    assert len(production['families'])==2 and len(production['candidates'])==6
    for row in production['families']:
        family=row['family']; report=export.decoded(export.read_bytes(Path(row['verification'])))
        assert report['source_sha256']==g.source_hashes()
        assert report['source_sha256_before']==report['source_sha256_after']==g.source_hashes()
        assert report['model_sha256']==export.sha(export.read_bytes(model,64*1024*1024))
        compiled=report['route_checks']['questions']
        assert report['route_checks']['routes']==18 and report['route_checks']['wrong_choices']==42
        assert report['route_checks']['save_replay'] and report['family_lesson_check']['accepted']
        assert sum(len(q['question']['steps']) for q in compiled)==21
        originals=[q for s in g.SETS for q in g.sequence(family,s)['questions']]
        assert len({q['id'] for q in originals})==18
        for i,set_name in enumerate(g.SETS):
            subset=compiled[i*6:i*6+6]
            positions=[next(j for j,o in enumerate(q['question']['steps'][0]['options'])
                            if o['id'] in q['question']['steps'][0]['accepted_option_ids']) for q in subset]
            assert positions==g.recipe()['families'][family][set_name]['first_positions']
            assert sorted(positions)==[0,0,1,1,2,2]
            patterns.append(tuple(positions))
            stage=next(r for r in production['candidates'] if r['family']==family and r['set']==set_name)
            staging=export.decoded(export.read_bytes(Path(stage['verification'])))
            # Exact shared receipt selection, not any previous check under this candidate.
            assert Path(stage['verification']).parent.name==export.sha(export.encoded(staging))
            assert staging['route_checks']['questions']==subset
        doc=export.read_bytes(Path(row['authoring'])/'documents/chapter.paths.md').decode()
        assert 'alpha names one known solution angle, which need not be acute' in doc
        if family=='sine_turn':
            assert 'pi-7pi/6=-pi/6; adding 2pi gives 11pi/6' in compiled[7]['question']['steps'][0]['explanation']
        assert doc.count('@lesson ')==1 and doc.count('@practice ')==18 and doc.count('@question ')==18
        assert doc.count('@read prod01_trigonometry_'+family+'_r')==18
        worked=(2,2,4) if family=='sine_turn' else (2,3,5)
        assert all(tuple(q['case'][k] for k in ('a','b','c'))!=worked for q in originals)
        rows.append(dict(family=family,first_positions=[list(t) for t in patterns[-3:]],staging_equal=True,
                         authoring=row['authoring'],verification=row['verification'],
                         verification_sha256=export.sha(export.read_bytes(Path(row['verification'])))))
    assert len(set(patterns))==6
    return rows


def main():
    before=g.source_hashes(); model=g.ROOT/'b/paths_learning_document_tests'
    production=export.decoded(export.read_bytes(g.OUT/'production.json'))
    records,rejections=check_whole_pool()
    mutations=compiler_mutations(model)
    families=final_receipts(production,model)
    assert before==g.source_hashes()
    report=dict(accepted=True,questions=36,readings=2,families=2,decisions=42,wrong_choices=84,
                source_sha256=before,model_sha256=export.sha(export.read_bytes(model,64*1024*1024)),
                production_receipt=production['immutable_receipt'],pool=records,
                boundary_rejections=rejections,compiler_mutations=mutations,consolidation=families,
                windows=0,published=False)
    encoded=export.encoded(report); path=g.OUT/'tests'/export.sha(encoded)/'verification.json'
    export.immutable_directory(path.parent,{'verification.json':encoded})
    (g.OUT/'tests.json').write_bytes(export.encoded(dict(accepted=True,verification=str(path))))
    print(json.dumps(dict(accepted=True,questions=36,readings=2,families=2,decisions=42,wrong_choices=84,
                         boundary_rejections=len(rejections),compiler_mutations=len(mutations),
                         verification=str(path)),indent=2))


if __name__=='__main__':
    main()
