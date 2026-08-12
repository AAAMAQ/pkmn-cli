from dataclasses import dataclass
from pathlib import Path

from .util import load_json


ROOT = Path(__file__).resolve().parents[1]


@dataclass(frozen=True)
class OriginalV1Policy:
    raw: dict
    salt: str

    @property
    def policy_id(self):
        return self.raw["policyId"]

    @property
    def version(self):
        return self.raw["policyVersion"]

    @property
    def profile_label(self):
        return f"{self.policy_id}@{self.version}"

    @classmethod
    def load(cls, salt=None, path=None):
        raw = load_json(path or ROOT / "data/pokemon_policy_original_v1.json")
        selected_salt = salt or raw["determinism"]["defaultSalt"]
        if not selected_salt:
            raise ValueError("Conversion salt must not be empty")
        return cls(raw=raw, salt=selected_salt)
