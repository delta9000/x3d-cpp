# Provider pilot migration

The earlier offline `offline_provider`/`provider_node` experiment and separate
hosted frontend have been replaced by one composed provider contract. This page
is retained as a navigation link, not another API specification.

Use [the portable contract](portable-contract.md), [runtime details](hosted-runtime.md),
and [the assertion migration ledger](provider-fixture-coverage-ledger.md).
`provider::service` owns a backend; setup and live operations use the same
scene/context/node/field family. Public SFNode/MFNode values carry provider
authority. The native backend continues to use authoritative native storage.

The old 1,199-check offline and later hosted proof are historical baselines.
Their assertions were migrated with explicit semantic replacements; current
validation is attributed to exact source inputs and paired runs. A larger check
count does not establish full ISO or component conformance.
