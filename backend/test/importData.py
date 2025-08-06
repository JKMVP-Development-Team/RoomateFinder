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
        reader = csv.DictReader(f)
        for row in reader:
            update_doc = {
                "$set": {
                    "address":       row.get("address", "").strip(),
                    "address_line":  row.get("address_line", "").strip(),
                    "city":          row.get("city", "").strip(),
                    "state":         row.get("state", "").strip(),
                    "country":       row.get("country", "").strip(),
                    "zipcode":       row.get("zipcode", "").strip(),
                    "email":         row.get("email", "").strip(),
                    "phone":         row.get("phone", "").strip()
                }
            }

            result = users.update_one({}, update_doc, upsert=True)
            if result.matched_count > 0:
                print(f"Updated user with data: {row}")
            else:
                print(f"Inserted new user with data: {row}")

if __name__ == "__main__":
    path = sys.argv[1] if len(sys.argv) > 1 else r"C:\\Users\\vinhp\Downloads\\MOCK_DATA.csv"
    import_and_update(path)