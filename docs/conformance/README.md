# Conformance reference

[INDEX.md](INDEX.md) presents the generated component view.
[findings.yaml](findings.yaml) owns behavioral judgments and their evidence.
The view does not certify normative conformance: detected runtime wiring does
not establish behavior required by ISO prose.

## Files

Edit the registers; generated files are outputs of the view generator.

| File | Role |
|---|---|
| [findings.yaml](findings.yaml) | Behavioral findings, status, clauses, and evidence |
| [profiles.yaml](profiles.yaml) | Profile-to-component requirements |
| [sai-services.yaml](sai-services.yaml) | Native SAI service coverage and test evidence |
| [INDEX.md](INDEX.md) | Generated component overview |
| [components/](components/) | Generated node and finding details |
| [model.json](model.json) | Generated machine-readable view |

## Judgments and derived facts

`scripts/conformance_view.py` joins the registers with the generated bindings,
interface registry, and runtime sources. System detection is heuristic.
Existence, extraction, and behavior are separate dimensions. Closed findings
record resolved gaps; they do not establish complete component conformance.

## Updating findings

Add or update a finding in `findings.yaml`. To close one, set `status: fixed`
or `status: closed` and record its test evidence in `note`. A resolving `commit`
is optional. Keep open acceptance conditions and deferral reasons intact.

Run `mise tasks` for register validation and regeneration tasks defined in
[mise.toml](../../mise.toml). Do not edit generated outputs by hand.

The [requirements audit](../wiki/guides/x3d4-requirements-audit.md) explains
profile requirements and the limits of the generated status labels.
