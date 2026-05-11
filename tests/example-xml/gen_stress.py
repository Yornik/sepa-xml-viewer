#!/usr/bin/env python3
"""Generate a pain.001.001.13 payroll stress-test fixture with 2500 transactions.

Deterministic (seeded). All IBANs are checksum-valid. All names / addresses /
identifiers are synthetic PLACEHOLDER values per tests/example-xml/README.md.
"""
import random
from pathlib import Path

random.seed(20260525)  # deterministic — re-runs produce identical bytes

N = 2500
OUTFILE = Path("tests/example-xml/pain.001.001.13-payroll-2500-stress.xml")


def iban_letter_to_num(s):
    return "".join(str(ord(c) - ord("A") + 10) if c.isalpha() else c for c in s)


def make_iban(country: str, bban: str) -> str:
    payload = iban_letter_to_num(bban) + iban_letter_to_num(country) + "00"
    check = 98 - int(payload) % 97
    iban = f"{country}{check:02d}{bban}"
    verify = iban_letter_to_num(iban[4:]) + iban_letter_to_num(iban[:4])
    assert int(verify) % 97 == 1
    return iban


# Realistic-looking but synthetic name pools — at least 50×50 = 2500 unique.
FIRST_NAMES_DE = [
    "Anna", "Bernd", "Clara", "David", "Eva", "Felix", "Greta", "Hans", "Ines",
    "Jonas", "Klara", "Lukas", "Maria", "Niklas", "Olga", "Paul", "Quirin",
    "Rosa", "Stefan", "Tilda", "Ulrich", "Vera", "Walter", "Xenia", "Yannik",
    "Zoe", "Andreas", "Beate", "Christian", "Dorothea", "Erik", "Franka",
    "Gerald", "Helene", "Ingo", "Julia", "Karsten", "Laura", "Matthias",
    "Nadine", "Oliver", "Petra", "Rainer", "Sabine", "Thomas", "Ute",
    "Volker", "Wibke", "Yvonne", "Sven",
]
LAST_NAMES_DE = [
    "Mueller", "Schmidt", "Schneider", "Fischer", "Weber", "Meyer", "Wagner",
    "Becker", "Schulz", "Hoffmann", "Schaefer", "Koch", "Bauer", "Richter",
    "Klein", "Wolf", "Schroeder", "Neumann", "Schwarz", "Zimmermann", "Krueger",
    "Hofmann", "Hartmann", "Lange", "Schmitt", "Werner", "Krause", "Lehmann",
    "Schmid", "Schulze", "Maier", "Koehler", "Herrmann", "Walter", "Mayer",
    "Huber", "Kaiser", "Fuchs", "Peters", "Lang", "Scholz", "Moeller",
    "Weiss", "Jung", "Hahn", "Schubert", "Vogel", "Friedrich", "Keller",
    "Guenther",
]
FIRST_NAMES_NL = ["Jan", "Pieter", "Sanne", "Femke", "Bram", "Lotte", "Daan", "Sophie"]
LAST_NAMES_NL = [
    "de Vries", "van der Berg", "van den Heuvel", "Jansen", "de Jong",
    "Visser", "Bakker", "Hoogendoorn",
]
FIRST_NAMES_FR = ["Pierre", "Marie", "Jean", "Camille", "Lucas", "Manon", "Antoine"]
LAST_NAMES_FR = ["Dubois", "Martin", "Bernard", "Laurent", "Petit", "Moreau", "Lefebvre"]

CITIES_DE = [
    ("Berlin", "10115"), ("Munich", "80331"), ("Hamburg", "20095"),
    ("Cologne", "50667"), ("Frankfurt", "60311"), ("Stuttgart", "70173"),
    ("Duesseldorf", "40213"), ("Leipzig", "04109"), ("Dortmund", "44135"),
    ("Essen", "45127"),
]
CITIES_NL = [("Amsterdam", "1016 GV"), ("Rotterdam", "3011 AB"), ("Utrecht", "3511 LH")]
CITIES_FR = [("Paris", "75001"), ("Lyon", "69001"), ("Marseille", "13001")]

STREETS_DE = ["Hauptstrasse", "Bahnhofstrasse", "Schillerstrasse", "Goethestrasse", "Lindenweg"]
STREETS_NL = ["Prinsengracht", "Keizersgracht", "Damrak", "Wilhelminakade"]
STREETS_FR = ["Rue de la Paix", "Avenue des Champs", "Boulevard Saint Michel"]

# 80% DE, 15% NL, 5% FR
def pick_country():
    r = random.random()
    if r < 0.80:
        return "DE"
    if r < 0.95:
        return "NL"
    return "FR"

def gen_employee(idx: int):
    country = pick_country()
    if country == "DE":
        nm = f"PLACEHOLDER {random.choice(FIRST_NAMES_DE)} {random.choice(LAST_NAMES_DE)}"
        bic = random.choice(["COBADEFFXXX", "DEUTDEFFXXX", "GENODEFFXXX", "HASPDEHHXXX"])
        # BLZ (8 digits, fake) + account (10 digits, sequential-ish)
        blz = f"3{random.randint(1000000, 9999999)}"  # 8-digit
        acct = f"{idx + 100000000:010d}"
        iban = make_iban("DE", blz + acct)
        city, postcode = random.choice(CITIES_DE)
        street = random.choice(STREETS_DE)
    elif country == "NL":
        nm = f"PLACEHOLDER {random.choice(FIRST_NAMES_NL)} {random.choice(LAST_NAMES_NL)}"
        bic = random.choice(["INGBNL2AXXX", "ABNANL2AXXX", "RABONL2UXXX"])
        # 4-letter bank code + 10-digit account
        bank_code = random.choice(["TEST", "PLAC", "MOCK", "ACME"])
        acct = f"{idx + 100000000:010d}"
        iban = make_iban("NL", bank_code + acct)
        city, postcode = random.choice(CITIES_NL)
        street = random.choice(STREETS_NL)
    else:  # FR
        nm = f"PLACEHOLDER {random.choice(FIRST_NAMES_FR)} {random.choice(LAST_NAMES_FR)}"
        bic = random.choice(["BNPAFRPPXXX", "CRLYFRPPXXX", "AGRIFRPPXXX"])
        # 5-digit bank + 5-digit branch + 11-alphanum account + 2-digit RIB
        bban = f"20041010050500013M{idx % 100:02d}{(idx * 7) % 100:02d}"
        bban = bban[:23]
        iban = make_iban("FR", bban)
        city, postcode = random.choice(CITIES_FR)
        street = random.choice(STREETS_FR)
    bldg = random.randint(1, 999)
    amount = round(random.uniform(1800.00, 4500.00), 2)
    return {
        "idx": idx, "nm": nm, "iban": iban, "bic": bic,
        "street": street, "bldg": bldg, "postcode": postcode, "city": city,
        "country": country, "amount": amount,
    }


employees = [gen_employee(i) for i in range(N)]
total_amount = round(sum(e["amount"] for e in employees), 2)

header = f"""<?xml version="1.0" encoding="UTF-8"?>
<!--
================================================================================
  SEPA Credit Transfer Initiation - pain.001.001.13
  STRESS TEST — {N}-transaction payroll batch
================================================================================

  ISO 20022 message:   pain.001.001.13
  Intent:              one debtor (Berlin GmbH) initiating a single payroll
                       run with {N} employee transactions. Used to exercise
                       the viewer's tree-rendering performance on a realistic
                       large file (a Fortune-500-size HR department).

                       Distribution: ~80% DE, ~15% NL, ~5% FR employees.
                       All names / IBANs / addresses are synthetic.

                       Control sum: EUR {total_amount:,.2f}

  >>>  TEST DATA - DO NOT USE FOR REAL PAYMENTS  <<<

  Generated by tests/example-xml/gen_stress.py (deterministic, seed=20260525).
  Every IBAN is checksum-valid. Every name is "PLACEHOLDER <first> <last>".
  See tests/example-xml/README.md for the placeholder convention.
================================================================================
-->
<Document xmlns="urn:iso:std:iso:20022:tech:xsd:pain.001.001.13"
          xmlns:xsi="http://www.w3.org/2001/XMLSchema-instance">
    <CstmrCdtTrfInitn>
        <GrpHdr>
            <MsgId>PLACEHOLDER-MSG-20260525-PAYROLL-STRESS-{N}</MsgId>
            <CreDtTm>2026-05-25T06:00:00</CreDtTm>
            <NbOfTxs>{N}</NbOfTxs>
            <CtrlSum>{total_amount:.2f}</CtrlSum>
            <InitgPty>
                <Nm>PLACEHOLDER MegaCorp International GmbH</Nm>
                <Id>
                    <OrgId>
                        <Othr>
                            <Id>PLACEHOLDER-ORG-ID-MEGACORP-001</Id>
                            <SchmeNm>
                                <Cd>BANK</Cd>
                            </SchmeNm>
                        </Othr>
                    </OrgId>
                </Id>
            </InitgPty>
        </GrpHdr>
        <PmtInf>
            <PmtInfId>PLACEHOLDER-PMT-INF-PAYROLL-STRESS-202605</PmtInfId>
            <PmtMtd>TRF</PmtMtd>
            <BtchBookg>true</BtchBookg>
            <NbOfTxs>{N}</NbOfTxs>
            <CtrlSum>{total_amount:.2f}</CtrlSum>
            <PmtTpInf>
                <SvcLvl><Cd>SEPA</Cd></SvcLvl>
                <CtgyPurp><Cd>SALA</Cd></CtgyPurp>
            </PmtTpInf>
            <ReqdExctnDt>
                <Dt>2026-05-25</Dt>
            </ReqdExctnDt>
            <Dbtr>
                <Nm>PLACEHOLDER MegaCorp International GmbH</Nm>
                <PstlAdr>
                    <StrtNm>PLACEHOLDER Unter den Linden</StrtNm>
                    <BldgNb>1</BldgNb>
                    <PstCd>10117</PstCd>
                    <TwnNm>Berlin</TwnNm>
                    <Ctry>DE</Ctry>
                </PstlAdr>
            </Dbtr>
            <DbtrAcct>
                <Id><IBAN>DE28500202000123456789</IBAN></Id>
                <Ccy>EUR</Ccy>
            </DbtrAcct>
            <DbtrAgt>
                <FinInstnId><BICFI>BANKDEFFXXX</BICFI></FinInstnId>
            </DbtrAgt>
            <ChrgBr>SLEV</ChrgBr>
"""

txn_template = """            <CdtTrfTxInf>
                <PmtId>
                    <InstrId>PLACEHOLDER-INSTR-PAY-{idx:05d}</InstrId>
                    <EndToEndId>PLACEHOLDER-E2E-SALARY-202605-EMP{idx:05d}</EndToEndId>
                </PmtId>
                <Amt>
                    <InstdAmt Ccy="EUR">{amount:.2f}</InstdAmt>
                </Amt>
                <CdtrAgt>
                    <FinInstnId><BICFI>{bic}</BICFI></FinInstnId>
                </CdtrAgt>
                <Cdtr>
                    <Nm>{nm}</Nm>
                    <PstlAdr>
                        <StrtNm>PLACEHOLDER {street}</StrtNm>
                        <BldgNb>{bldg}</BldgNb>
                        <PstCd>{postcode}</PstCd>
                        <TwnNm>{city}</TwnNm>
                        <Ctry>{country}</Ctry>
                    </PstlAdr>
                </Cdtr>
                <CdtrAcct>
                    <Id><IBAN>{iban}</IBAN></Id>
                </CdtrAcct>
                <RmtInf>
                    <Ustrd>PLACEHOLDER Salary May 2026 — Employee {idx:05d}</Ustrd>
                </RmtInf>
            </CdtTrfTxInf>
"""

footer = """        </PmtInf>
    </CstmrCdtTrfInitn>
</Document>
"""

OUTFILE.parent.mkdir(parents=True, exist_ok=True)
with OUTFILE.open("w", encoding="utf-8") as f:
    f.write(header)
    for e in employees:
        f.write(txn_template.format(**e))
    f.write(footer)

print(f"Wrote {OUTFILE} ({OUTFILE.stat().st_size:,} bytes, {N} transactions)")
