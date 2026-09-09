#!/usr/bin/env python3
"""Prepare one explicitly reviewed source lesson for export_learning.py.

Uses the existing transcript parser's public, lossless github profile. The
review selects source bytes and supplies presentation; no headings, solutions,
learner attempts or mathematical answers are inferred from arbitrary Markdown.
"""
from __future__ import annotations

import argparse
import importlib
from pathlib import Path
import re
import sys

from export_learning import (ROOT, ExportError, capture, decoded, disjoint, encoded,
                             identity, immutable_directory, read_bytes, real_path,
                             relative, require, sha)


def prepared_route(card, display):
    """Retain the current prepared route's decisions; substitute reviewed TeX only."""
    identity(display["question_id"])
    states = display["states"]
    require(set(states) == {str(s["id"]) for s in card["working_states"]},
            "mapping.states", "Presentation must cover every current working state exactly")
    require(set(display["choices"]) == {str(s["id"]) for s in card["steps"]},
            "mapping.steps", "Presentation must cover every current step exactly")
    lines = [f'@question {display["question_id"]} | {display["title"]}',
             "@template choices.v1", f'@version {card["content_version"]}',
             f'@goal {card["description"]}', f'@given {states[str(card["steps"][0]["semantics"]["before"])]}',
             f'@domain {display["domain"]}']
    previous = card["steps"][0]["semantics"]["before"]
    for step in card["steps"]:
        require(step["semantics"]["before"] == previous and len(step["accepted_option_ids"]) == 1
                and step["semantics"]["completion"] == "any_accepted",
                "mapping.route", "This adapter needs a sequential prepared route with one accepted option per step")
        labels = display["choices"][str(step["id"])]
        require(set(labels) == {str(o["id"]) for o in step["options"]},
                "mapping.choices", f'Presentation must cover every choice in step {step["id"]}')
        lines.append(f'@step {step["id"]} | {step["prompt"]}')
        for option in step["options"]:
            label = labels[str(option["id"])]
            require(set(label) in ({"math"}, {"text"}), "mapping.label", "Choose math or text for each label")
            key = next(iter(label))
            command = "choice" if key == "math" else "textchoice"
            lines.append(f'@{command} {option["id"]} | {label[key]}')
        previous = step["semantics"]["after"]
        lines.extend([f'@answer {step["accepted_option_ids"][0]}', f'@after {states[str(previous)]}',
                      f'@why {step["explanation"]}', f'@wrong {step["wrong_hint"]}'])
    lines.append("@end")
    return "\n".join(lines) + "\n"


def prepare(review_path, source_root, parser_root, output):
    review_path, source_root, parser_root, output = map(real_path, (review_path, source_root, parser_root, output))
    review_bytes = read_bytes(review_path)
    review = decoded(review_bytes)
    require(review["format"] == "paths_source_lesson_review" and type(review["format_version"]) is int and review["format_version"] == 1
            and review["status"] == "reviewed", "source.review", "Only a reviewed version-1 mapping can be prepared")
    roots = {"source": source_root, "repo": ROOT, "review": review_path.parent}
    inputs, paths = {}, {}
    require(0 < len(review["inputs"]) <= 32, "source.inputs", "Select 1-32 explicit inputs")
    for item in review["inputs"]:
        key = identity(item["id"])
        require(key not in inputs and item["root"] in roots, "source.inputs", "Repeated input or unknown root")
        path = roots[item["root"]] / relative(item["path"])
        data = read_bytes(path, 128 * 1024)
        if sha(data) != item["sha256"]:
            raise ExportError("source.stale", f'{item["path"]}: reviewed {key} changed; inspect it and review the mapping again',
                              [dict(file=item["path"], field=f'inputs.{key}.sha256', code="source.stale",
                                    expected=item["sha256"], actual=sha(data))])
        inputs[key], paths[key] = data, path
    disjoint(output, [review_path.parent, source_root, parser_root, *paths.values()])
    source = inputs["source_card"]
    selection = review["question_selection"]
    start, end = selection["start_byte"], selection["end_byte"]
    require(type(start) is int and type(end) is int and 0 <= start < end <= len(source),
            "source.selection", "Question selection must be a bounded byte range")
    question = source[start:end]
    require(sha(question) == selection["sha256"], "source.selection", "Reviewed question selection changed")
    # Do not write caches into the independently maintained parser checkout.
    sys.dont_write_bytecode = True
    sys.path.insert(0, str(parser_root))
    parser = importlib.import_module("transcript_parser")
    require(real_path(parser.__file__).is_relative_to(parser_root) and parser.__version__ == review["parser_version"],
            "parser.version", "Use the reviewed transcript-parser checkout/version")
    parsed = parser.Parser(parser.ParseOptions(profile="github")).parse(
        source.decode("utf-8"), input_format="text", source_name=paths["source_card"].name)
    require(parsed.output.encode("utf-8") == source, "parser.changed", "Lossless source audit changed the original bytes")
    audit = parsed.to_dict()
    require(audit["report"]["schema_version"] == 1 and audit["report"]["parser_version"] == parser.__version__
            and audit["report"]["input_sha256"] == sha(source), "parser.audit", "Source audit schema or identity differs from the reviewed contract")
    substitutions = {"prepared_route": prepared_route(decoded(inputs["prepared_card"]), decoded(inputs["presentation"]))}
    references = decoded(inputs["row_rules"])["references"]
    # Use existing, current definitions, in an explicitly reviewed selection order.
    for key in review["row_definitions"]:
        found = [r for r in references if r["id"] == key]
        require(len(found) == 1, "mapping.definition", f"Unknown or repeated row definition: {key}")
        substitutions[key] = found[0]["definition"]
    files = {"authoring.json": encoded(review["authoring"]),
             "audit/review.json": review_bytes,
             "audit/source.md": source,
             "audit/question.md": question,
             "audit/parser.json": encoded(audit)}
    for item in review["documents"]:
        name = "documents/" + relative(item["path"])
        require(name not in files and name.endswith(".md"), "mapping.document", "Repeated or non-Markdown document")
        text = inputs[item["input"]].decode("utf-8")
        def expand(match):
            key = match.group(1)
            require(key in substitutions, "mapping.command", f"Unknown reviewed substitution: {key}")
            return substitutions[key]
        files[name] = re.sub(r"\{\{([a-z][a-z0-9_]*)\}\}", expand, text).encode("utf-8")
    # A change during parser/formatting work must not install a mixed generation.
    for key, path in paths.items():
        require(read_bytes(path) == inputs[key], "source.changed", f"Input changed during preparation: {path.name}")
    require(read_bytes(review_path) == review_bytes, "source.changed", "Review changed during preparation")
    if output.exists():
        require(capture(output) == files, "source.immutable", "Prepared output differs; choose a new version/output after review")
    else:
        immutable_directory(output, files)
    return dict(accepted=True, output=str(output), package_id=review["authoring"]["package_id"],
                source_sha256=sha(source), review_sha256=sha(review_bytes), parser_version=parser.__version__,
                lossless=True, exported=False, published=False)


def main():
    cli = argparse.ArgumentParser(description=__doc__)
    cli.add_argument("review", type=Path)
    cli.add_argument("--source-root", required=True, type=Path)
    cli.add_argument("--parser-root", required=True, type=Path)
    cli.add_argument("--output", required=True, type=Path)
    args = cli.parse_args()
    try:
        sys.stdout.buffer.write(encoded(prepare(args.review, args.source_root, args.parser_root, args.output)))
        return 0
    except (ExportError, OSError, ValueError, KeyError, TypeError, ImportError) as error:
        sys.stderr.buffer.write(encoded(dict(accepted=False, code=getattr(error, "code", "source.failed"),
                                            message=str(error), diagnostics=getattr(error, "diagnostics", []))))
        return 1


if __name__ == "__main__":
    raise SystemExit(main())
