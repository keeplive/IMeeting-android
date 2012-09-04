package com.richitec.imeeting.talkinggroup;

import java.util.ArrayList;
import java.util.List;

import org.json.JSONArray;
import org.json.JSONException;
import org.json.JSONObject;

import android.content.Context;
import android.text.format.DateFormat;
import android.util.Log;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.BaseAdapter;
import android.widget.RelativeLayout;
import android.widget.TableRow;
import android.widget.TextView;

import com.richitec.imeeting.R;
import com.richitec.imeeting.constants.SystemConstants;
import com.richitec.imeeting.constants.TalkGroup;

public class TalkingGroupListAdapter extends BaseAdapter {
	private LayoutInflater inflater;
	private List<JSONObject> groups;

	public TalkingGroupListAdapter(Context context) {
		inflater = LayoutInflater.from(context);
		groups = new ArrayList<JSONObject>();
	}
	
	public void setGroupList(JSONArray groupArray) {
		groups.clear();
		if (groupArray != null) {
			for (int i = 0; i < groupArray.length(); i++) {
				try {
					JSONObject obj = groupArray.getJSONObject(i);
					groups.add(obj);
				} catch (JSONException e) {
					e.printStackTrace();
				}
			}
		}
		notifyDataSetChanged();
	}

	public void clear() {
		groups.clear();
		notifyDataSetChanged();
	}

	@Override
	public int getCount() {
		return groups.size();
	}

	@Override
	public Object getItem(int position) {
		Object obj = null;
		obj = groups.get(position);
		return obj;
	}

	@Override
	public long getItemId(int position) {
		return position;
	}

	@Override
	public View getView(int position, View convertView, ViewGroup parent) {
		ViewHolder viewHolder = null;
		if (convertView == null) {
			viewHolder = new ViewHolder();
			convertView = inflater.inflate(R.layout.talking_group_history_list_item_layout, null);

			viewHolder.titleTV = (TextView) convertView
					.findViewById(R.id.talkingGroupTitle_textView);
			viewHolder.createdTimeTV = (TextView) convertView.findViewById(R.id.talkingGroup_createdTime_textView);
			viewHolder.memberRow = (TableRow) convertView.findViewById(R.id.talkingGroup_attendees_tableRow);
			convertView.setTag(viewHolder);
		} else {
			viewHolder = (ViewHolder) convertView.getTag();
		}

		JSONObject groupItem = (JSONObject) getItem(position);

		try {
			viewHolder.titleTV.setText(groupItem.getString(TalkGroup.title
					.name()));
			
			long createdTime = groupItem.getLong(TalkGroup.created_time.name());
			CharSequence date = DateFormat.format("yyyy-MM-dd hh:mm", createdTime * 1000);
			viewHolder.createdTimeTV.setText(date);
			
			JSONArray attendees = groupItem.getJSONArray(TalkGroup.attendees.name());
			for (int i = 0; i < viewHolder.memberRow.getVirtualChildCount(); i++) {
				// get table row item
				View _tableRowItem = viewHolder.memberRow.getVirtualChildAt(i);

				// check visible view
				if (i < attendees.length()) {
					// check table row item type
					// linearLayout
					String attendeePhoneNumber = attendees.getString(i);
					if (_tableRowItem instanceof RelativeLayout) {
						// set attendee name
						((TextView) ((RelativeLayout) _tableRowItem)
								.findViewById(R.id.attendee_name_textView))
								.setText(attendeePhoneNumber);
					}
				} else {
					_tableRowItem.setVisibility(View.GONE);
				}
			}
			
			String status = groupItem.getString(TalkGroup.status.name());
			if (status.equals("OPEN")) {
				viewHolder.titleTV.setTextColor(0xff8fbc8f);
			} else {
				viewHolder.titleTV.setTextColor(0xffa3a3a3);
			}
		} catch (JSONException e) {
			e.printStackTrace();
		}

		return convertView;
	}

	final class ViewHolder {
		public TextView titleTV;
		public TextView createdTimeTV;
		public TableRow memberRow;
	}
}