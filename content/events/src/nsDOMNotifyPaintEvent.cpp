/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* ***** BEGIN LICENSE BLOCK *****
 * Version: MPL 1.1/GPL 2.0/LGPL 2.1
 *
 * The contents of this file are subject to the Mozilla Public License Version
 * 1.1 (the "License"); you may not use this file except in compliance with
 * the License. You may obtain a copy of the License at
 * http://www.mozilla.org/MPL/
 *
 * Software distributed under the License is distributed on an "AS IS" basis,
 * WITHOUT WARRANTY OF ANY KIND, either express or implied. See the License
 * for the specific language governing rights and limitations under the
 * License.
 *
 * The Original Code is mozilla.org code.
 *
 * The Initial Developer of the Original Code is 
 * Mozilla Corporation
 * Portions created by the Initial Developer are Copyright (C) 2008
 * the Initial Developer. All Rights Reserved.
 *
 * Contributor(s):
 *  Robert O'Callahan <robert@ocallahan.org>
 *
 * Alternatively, the contents of this file may be used under the terms of
 * either of the GNU General Public License Version 2 or later (the "GPL"),
 * or the GNU Lesser General Public License Version 2.1 or later (the "LGPL"),
 * in which case the provisions of the GPL or the LGPL are applicable instead
 * of those above. If you wish to allow use of your version of this file only
 * under the terms of either the GPL or the LGPL, and not to allow others to
 * use your version of this file under the terms of the MPL, indicate your
 * decision by deleting the provisions above and replace them with the notice
 * and other provisions required by the GPL or the LGPL. If you do not delete
 * the provisions above, a recipient may use your version of this file under
 * the terms of any one of the MPL, the GPL or the LGPL.
 *
 * ***** END LICENSE BLOCK ***** */

#include "nsDOMNotifyPaintEvent.h"
#include "nsContentUtils.h"
#include "nsClientRect.h"
#include "nsPaintRequest.h"
#include "nsIFrame.h"

nsDOMNotifyPaintEvent::nsDOMNotifyPaintEvent(nsPresContext* aPresContext,
                                             nsEvent* aEvent,
                                             PRUint32 aEventType,
                                             nsInvalidateRequestList* aInvalidateRequests)
: nsDOMEvent(aPresContext, aEvent)
{
  if (mEvent) {
    mEvent->message = aEventType;
  }
  if (aInvalidateRequests) {
    mInvalidateRequests.SwapElements(aInvalidateRequests->mRequests);
  }
}

NS_INTERFACE_MAP_BEGIN(nsDOMNotifyPaintEvent)
  NS_INTERFACE_MAP_ENTRY(nsIDOMNotifyPaintEvent)
  NS_INTERFACE_MAP_ENTRY_CONTENT_CLASSINFO(NotifyPaintEvent)
NS_INTERFACE_MAP_END_INHERITING(nsDOMEvent)

NS_IMPL_ADDREF_INHERITED(nsDOMNotifyPaintEvent, nsDOMEvent)
NS_IMPL_RELEASE_INHERITED(nsDOMNotifyPaintEvent, nsDOMEvent)

nsRegion
nsDOMNotifyPaintEvent::GetRegion()
{
  nsRegion r;
  PRBool isTrusted = nsContentUtils::IsCallerTrustedForRead();
  for (PRUint32 i = 0; i < mInvalidateRequests.Length(); ++i) {
    if (!isTrusted &&
        (mInvalidateRequests[i].mFlags & nsIFrame::INVALIDATE_CROSS_DOC))
      continue;

    r.Or(r, mInvalidateRequests[i].mRect);
    r.SimplifyOutward(10);
  }
  return r;
}

NS_IMETHODIMP
nsDOMNotifyPaintEvent::GetBoundingClientRect(nsIDOMClientRect** aResult)
{
  // Weak ref, since we addref it below
  nsClientRect* rect = new nsClientRect();
  if (!rect)
    return NS_ERROR_OUT_OF_MEMORY;

  NS_ADDREF(*aResult = rect);
  if (!mPresContext)
    return NS_OK;

  rect->SetLayoutRect(GetRegion().GetBounds());
  return NS_OK;
}

NS_IMETHODIMP
nsDOMNotifyPaintEvent::GetClientRects(nsIDOMClientRectList** aResult)
{
  nsRefPtr<nsClientRectList> rectList = new nsClientRectList();
  if (!rectList)
    return NS_ERROR_OUT_OF_MEMORY;

  nsRegion r = GetRegion();
  nsRegionRectIterator iter(r);
  for (const nsRect* rgnRect = iter.Next(); rgnRect; rgnRect = iter.Next()) {
    nsRefPtr<nsClientRect> rect = new nsClientRect();
    if (!rect)
      return NS_ERROR_OUT_OF_MEMORY;
    
    rect->SetLayoutRect(*rgnRect);
    rectList->Append(rect);
  }

  rectList.forget(aResult);
  return NS_OK;
}

NS_IMETHODIMP
nsDOMNotifyPaintEvent::GetPaintRequests(nsIDOMPaintRequestList** aResult)
{
  nsRefPtr<nsPaintRequestList> requests = new nsPaintRequestList();
  if (!requests)
    return NS_ERROR_OUT_OF_MEMORY;

  PRBool isTrusted = nsContentUtils::IsCallerTrustedForRead();
  for (PRUint32 i = 0; i < mInvalidateRequests.Length(); ++i) {
    if (!isTrusted &&
        (mInvalidateRequests[i].mFlags & nsIFrame::INVALIDATE_CROSS_DOC))
      continue;

    nsRefPtr<nsPaintRequest> r = new nsPaintRequest();
    if (!r)
      return NS_ERROR_OUT_OF_MEMORY;
    r->SetRequest(mInvalidateRequests[i]);
    requests->Append(r);
  }

  requests.forget(aResult);
  return NS_OK;
}

nsresult NS_NewDOMNotifyPaintEvent(nsIDOMEvent** aInstancePtrResult,
                                   nsPresContext* aPresContext,
                                   nsEvent *aEvent,
                                   PRUint32 aEventType,
                                   nsInvalidateRequestList* aInvalidateRequests) 
{
  nsDOMNotifyPaintEvent* it =
    new nsDOMNotifyPaintEvent(aPresContext, aEvent, aEventType,
                              aInvalidateRequests);
  if (nsnull == it) {
    return NS_ERROR_OUT_OF_MEMORY;
  }

  return CallQueryInterface(it, aInstancePtrResult);
}
