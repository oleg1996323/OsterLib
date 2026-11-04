#pragma once

namespace linalgebra{
    struct Strategy{};

    struct ElementMultiplies:Strategy{};
    struct CrossMultiplies:Strategy{};
    struct ExternalMultiplies:Strategy{};
    struct ScalarMultiplies:Strategy{};
}