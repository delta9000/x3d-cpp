# Conformance registers

[findings.yaml](findings.yaml) owns behavioral judgments and their evidence;
everything else here is generated from it or read by the generator. Edit the
registers, then run `mise run conformance`. Never edit generated files by hand.

| File | Role |
|---|---|
| [findings.yaml](findings.yaml) | Behavioral findings, status, clauses and evidence (edit) |
| [profiles.yaml](profiles.yaml) | Profile-to-component requirements (edit) |
| [sai-services.yaml](sai-services.yaml) | Native Script-SAI service coverage and test evidence (edit) |
| [INDEX.md](INDEX.md), [components/](components/), [model.json](model.json) | Generated view |
| [REPORT.md](REPORT.md) | Generated structural corpus report |

How to add, close or defer a finding:
[Audit a requirement](../wiki/how-to/audit-a-requirement.md). What the view
does and does not establish:
[Conformance infrastructure](../wiki/reference/subsystems/conformance-infra.md).
