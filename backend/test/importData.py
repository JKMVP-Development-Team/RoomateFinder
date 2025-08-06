import csv
import sys
from pymongo import MongoClient
from os import getenv

def import_and_update(csv_path: str,
                      mongo_uri: str = getenv("MONGODB_URI"),
                      db_name: str = "roommatefinder",
                      coll_name: str = "users"):
    client = MongoClient(mongo_uri)
    db     = client[db_name]
    users  = db[coll_name]
    with open(csv_path, newline="", encoding="utf-8") as f:
        rows = list(csv.DictReader(f))
    docs = list(users.find({}, {"_id": 1}))
    
    for row, doc in zip(rows, docs):
        update_fields = {
            "address":      row.get("address", "").strip(),
            "address_line": row.get("address_line", "").strip(),
            "city":         row.get("city", "").strip(),
            "state":        row.get("state", "").strip(),
            "country":      row.get("country", "").strip(),
            "zipcode":      row.get("zipcode", "").strip(),
            "phone":        row.get("phone", "").strip(),
            "email":        row.get("email", "").strip(),
        }

        result = users.update_one(
            {"_id": doc["_id"]},
            {"$set": update_fields,
             "$unset": { "lat": "", "long": "" }
            }
        )
        print(f"_id={doc['_id']}  matched={result.matched_count}  modified={result.modified_count}")

if __name__ == "__main__":
    path = sys.argv[1] if len(sys.argv) > 1 else r"backend\\test\\MOCK_DATA.csv"
    import_and_update(path)